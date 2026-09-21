#pragma once

#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace catalog {

// A single SQLite connection, opened through Qt's bundled QSQLITE driver.
//
// One connection per thread, never shared: QSqlDatabase handles are tied to the
// thread that opened them, and WAL only makes concurrent *connections* safe, not
// concurrent use of one handle. Each Database instance registers its own
// uniquely-named Qt connection, so several can coexist over the same file.
class Database {
public:
    struct Options {
        // Applied before any other PRAGMA and before any DDL. SQLite can only
        // honor these on an empty database file: page_size otherwise needs a
        // full VACUUM to take effect, and auto_vacuum cannot be raised from
        // NONE at all without one. 0 leaves the driver default in place.
        int pageSize = 0;
        bool incrementalAutoVacuum = false;

        bool walMode = true;
        bool foreignKeys = true;
        int busyTimeoutMs = 5000;
        bool readOnly = false;
    };

    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // `path` may be ":memory:" for a private in-memory database, which is what
    // the tests use. Options that only apply to a file (page_size, WAL) are
    // skipped in that case rather than failing.
    bool open(const QString& path, const Options& options);

    // Spelled as an overload rather than a defaulted argument: a default
    // argument of `Options()` would need the nested struct's member
    // initializers before the enclosing class is complete.
    bool open(const QString& path) { return open(path, Options()); }
    void close();

    bool isOpen() const;
    QString path() const { return dbPath; }
    QSqlDatabase handle() const;

    // Runs a statement that takes no parameters and returns no rows. On failure
    // the driver's message is recorded in lastError() and logged.
    bool exec(const QString& sql);

    // Runs several statements in one transaction, stopping at the first
    // failure. Used for schema creation.
    bool execBatch(const QStringList& statements);

    // Single-value query; returns `fallback` on any failure or empty result.
    qint64 scalar(const QString& sql, qint64 fallback = 0);

    // Prepared statement bound to this connection. Always check isValid() on
    // the returned query — a prepare failure is reported here, not at exec().
    QSqlQuery prepare(const QString& sql);

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    bool inTransaction() const { return transactionActive; }

    QString lastError() const { return errorText; }
    void setError(const QString& text) { errorText = text; }

private:
    bool applyPragmas(const Options& options);

    QString connectionName;
    QString dbPath;
    QString errorText;
    bool opened = false;
    bool transactionActive = false;
};

// RAII transaction. Rolls back on destruction unless commit() succeeded, so an
// early return or a failed step can never leave a half-written batch behind.
//
// Does not nest: constructing one while another is active is a programming
// error, and the inner instance becomes inert rather than committing the outer
// transaction out from under it.
class Transaction {
public:
    explicit Transaction(Database& db);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    bool isActive() const { return active; }
    bool commit();
    void rollback();

private:
    Database* db = nullptr;
    bool active = false;
};

} // namespace catalog
