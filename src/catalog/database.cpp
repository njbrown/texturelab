#include "database.h"

#include <QAtomicInteger>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QThread>
#include <QVariant>

namespace catalog {

namespace {

// Qt keys connections by name in a process-wide registry, so every Database
// instance needs its own. The thread id is in the name purely to make a
// cross-thread misuse obvious in a debugger.
QString makeConnectionName()
{
    static QAtomicInteger<quint64> counter(0);
    return QStringLiteral("texturelab_catalog_%1_%2")
        .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16)
        .arg(counter.fetchAndAddRelaxed(1));
}

bool isMemoryPath(const QString& path)
{
    return path == QLatin1String(":memory:") || path.startsWith(QLatin1String("file::memory:"));
}

// Guards addDatabase/removeDatabase only. Qt's connection registry is a
// process-wide map, and the reconciliation pass opens its own connections from
// a worker thread while the GUI thread holds its own. Individual connections
// stay thread-confined; this just keeps two threads from mutating the registry
// at the same moment.
QMutex& registryMutex()
{
    static QMutex mutex;
    return mutex;
}

} // namespace

Database::Database() = default;

Database::~Database()
{
    close();
}

bool Database::open(const QString& path, const Options& options)
{
    close();

    connectionName = makeConnectionName();
    dbPath = path;

    // Every QSqlDatabase copy must be destroyed before removeDatabase() runs,
    // or Qt warns that the connection is still in use and leaves it registered.
    // That includes the failure paths below, hence the scoping.
    bool openFailed = false;
    {
        QMutexLocker lock(&registryMutex());
        QSqlDatabase conn = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        conn.setDatabaseName(path);

        QStringList connectOptions;
        connectOptions << QStringLiteral("QSQLITE_BUSY_TIMEOUT=%1").arg(options.busyTimeoutMs);
        if (options.readOnly)
            connectOptions << QStringLiteral("QSQLITE_OPEN_READONLY");
        conn.setConnectOptions(connectOptions.join(QLatin1Char(';')));

        if (!conn.open()) {
            errorText = conn.lastError().text();
            openFailed = true;
        }
    }

    if (openFailed) {
        qWarning("catalog: could not open %s: %s", qPrintable(path), qPrintable(errorText));
        {
            QMutexLocker lock(&registryMutex());
            QSqlDatabase::removeDatabase(connectionName);
        }
        connectionName.clear();
        return false;
    }

    opened = true;

    if (!applyPragmas(options)) {
        close();
        return false;
    }

    return true;
}

void Database::close()
{
    if (!connectionName.isEmpty()) {
        if (transactionActive)
            rollbackTransaction();
        {
            QSqlDatabase db = QSqlDatabase::database(connectionName, false);
            if (db.isValid() && db.isOpen())
                db.close();
        }
        // The QSqlDatabase copy above must be out of scope before
        // removeDatabase, or Qt warns about a connection still in use.
        {
            QMutexLocker lock(&registryMutex());
            QSqlDatabase::removeDatabase(connectionName);
        }
        connectionName.clear();
    }
    opened = false;
    transactionActive = false;
    dbPath.clear();
}

bool Database::isOpen() const
{
    return opened && handle().isOpen();
}

QSqlDatabase Database::handle() const
{
    return QSqlDatabase::database(connectionName, false);
}

bool Database::applyPragmas(const Options& options)
{
    const bool memory = isMemoryPath(dbPath);

    // Order matters and is not negotiable. page_size and auto_vacuum can only
    // take effect on a database with no pages yet, and journal_mode=WAL writes
    // a page — so both must precede it. Getting this backwards fails silently:
    // the PRAGMA reports success and the setting simply doesn't apply.
    if (!options.readOnly && !memory) {
        if (options.pageSize > 0
            && !exec(QStringLiteral("PRAGMA page_size = %1").arg(options.pageSize)))
            return false;

        if (options.incrementalAutoVacuum && !exec(QStringLiteral("PRAGMA auto_vacuum = INCREMENTAL")))
            return false;

        if (options.walMode) {
            if (!exec(QStringLiteral("PRAGMA journal_mode = WAL")))
                return false;
            if (!exec(QStringLiteral("PRAGMA synchronous = NORMAL")))
                return false;
        }
    }

    if (options.foreignKeys && !exec(QStringLiteral("PRAGMA foreign_keys = ON")))
        return false;

    return true;
}

bool Database::exec(const QString& sql)
{
    QSqlQuery query(handle());
    if (!query.exec(sql)) {
        errorText = query.lastError().text();
        qWarning("catalog: query failed: %s [%s]", qPrintable(errorText), qPrintable(sql));
        return false;
    }
    return true;
}

bool Database::execBatch(const QStringList& statements)
{
    Transaction tx(*this);
    if (!tx.isActive())
        return false;

    for (const QString& sql : statements) {
        if (!exec(sql))
            return false;
    }

    return tx.commit();
}

qint64 Database::scalar(const QString& sql, qint64 fallback)
{
    QSqlQuery query(handle());
    if (!query.exec(sql) || !query.next())
        return fallback;

    const QVariant value = query.value(0);
    return value.isNull() ? fallback : value.toLongLong();
}

QSqlQuery Database::prepare(const QString& sql)
{
    QSqlQuery query(handle());
    if (!query.prepare(sql)) {
        errorText = query.lastError().text();
        qWarning("catalog: prepare failed: %s [%s]", qPrintable(errorText), qPrintable(sql));
    }
    return query;
}

bool Database::beginTransaction()
{
    if (transactionActive) {
        errorText = QStringLiteral("transaction already active");
        return false;
    }
    // BEGIN IMMEDIATE takes the write lock up front. The default deferred
    // transaction takes it at the first write, which under WAL can fail with
    // SQLITE_BUSY partway through a batch that already read data — the classic
    // "upgrade deadlock" that busy_timeout cannot resolve.
    if (!exec(QStringLiteral("BEGIN IMMEDIATE")))
        return false;

    transactionActive = true;
    return true;
}

bool Database::commitTransaction()
{
    if (!transactionActive) {
        errorText = QStringLiteral("no transaction to commit");
        return false;
    }
    const bool ok = exec(QStringLiteral("COMMIT"));
    transactionActive = false;
    return ok;
}

bool Database::rollbackTransaction()
{
    if (!transactionActive)
        return false;

    const bool ok = exec(QStringLiteral("ROLLBACK"));
    transactionActive = false;
    return ok;
}

Transaction::Transaction(Database& database) : db(&database)
{
    if (db->inTransaction()) {
        qWarning("catalog: nested transaction requested; inner scope is inert");
        return;
    }
    active = db->beginTransaction();
}

Transaction::~Transaction()
{
    if (active)
        rollback();
}

bool Transaction::commit()
{
    if (!active)
        return false;

    active = false;
    return db->commitTransaction();
}

void Transaction::rollback()
{
    if (!active)
        return;

    active = false;
    db->rollbackTransaction();
}

} // namespace catalog
