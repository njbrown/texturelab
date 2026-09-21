#include "database.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

using namespace catalog;

class TestDatabase : public QObject {
    Q_OBJECT

private slots:
    void init();

    void opensFileAndReportsOpen();
    void appliesPageSizeBeforeAnyDdl();
    void appliesIncrementalAutoVacuum();
    void enablesWalMode();
    void enablesForeignKeys();
    void memoryDatabaseSkipsFilePragmas();

    void transactionCommitPersists();
    void transactionRollsBackWhenScopeExits();
    void transactionRollsBackOnExplicitCall();
    void nestedTransactionIsInertAndDoesNotCommitOuter();

    void execBatchIsAtomic();
    void scalarReturnsFallbackOnFailure();
    void readOnlyConnectionRejectsWrites();

    void separateConnectionsCanReadConcurrently();

private:
    QString dbFile(const QString& name) const { return dir.filePath(name); }

    QTemporaryDir dir;
};

void TestDatabase::init()
{
    QVERIFY(dir.isValid());
}

void TestDatabase::opensFileAndReportsOpen()
{
    Database db;
    QVERIFY(db.open(dbFile(QStringLiteral("open.db"))));
    QVERIFY(db.isOpen());

    db.close();
    QVERIFY(!db.isOpen());
}

void TestDatabase::appliesPageSizeBeforeAnyDdl()
{
    // The ordering this asserts is the whole reason Database applies PRAGMAs
    // itself: page_size only takes on an empty file. If a future refactor
    // creates a table first, this drops back to the 4096 default and the
    // setting is silently lost.
    const QString path = dbFile(QStringLiteral("pagesize.db"));

    Database::Options options;
    options.pageSize = 8192;

    Database db;
    QVERIFY(db.open(path, options));
    QVERIFY(db.exec(QStringLiteral("CREATE TABLE t (a INTEGER)")));

    QCOMPARE(db.scalar(QStringLiteral("PRAGMA page_size")), 8192LL);

    // And it survives a reopen, i.e. it was written into the file header
    // rather than just held in the connection.
    db.close();
    Database reopened;
    QVERIFY(reopened.open(path));
    QCOMPARE(reopened.scalar(QStringLiteral("PRAGMA page_size")), 8192LL);
}

void TestDatabase::appliesIncrementalAutoVacuum()
{
    const QString path = dbFile(QStringLiteral("autovacuum.db"));

    Database::Options options;
    options.incrementalAutoVacuum = true;

    Database db;
    QVERIFY(db.open(path, options));
    QVERIFY(db.exec(QStringLiteral("CREATE TABLE t (a INTEGER)")));

    // 0 = NONE, 1 = FULL, 2 = INCREMENTAL.
    QCOMPARE(db.scalar(QStringLiteral("PRAGMA auto_vacuum")), 2LL);

    db.close();
    Database reopened;
    QVERIFY(reopened.open(path));
    QCOMPARE(reopened.scalar(QStringLiteral("PRAGMA auto_vacuum")), 2LL);
}

void TestDatabase::enablesWalMode()
{
    Database db;
    QVERIFY(db.open(dbFile(QStringLiteral("wal.db"))));

    QSqlQuery query = db.prepare(QStringLiteral("PRAGMA journal_mode"));
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString().toLower(), QStringLiteral("wal"));
}

void TestDatabase::enablesForeignKeys()
{
    Database db;
    QVERIFY(db.open(dbFile(QStringLiteral("fk.db"))));
    QCOMPARE(db.scalar(QStringLiteral("PRAGMA foreign_keys")), 1LL);

    QVERIFY(db.exec(QStringLiteral("CREATE TABLE parent (id INTEGER PRIMARY KEY)")));
    QVERIFY(db.exec(QStringLiteral(
        "CREATE TABLE child (id INTEGER PRIMARY KEY, "
        "parent_id INTEGER REFERENCES parent(id) ON DELETE CASCADE)")));
    QVERIFY(db.exec(QStringLiteral("INSERT INTO parent (id) VALUES (1)")));
    QVERIFY(db.exec(QStringLiteral("INSERT INTO child (id, parent_id) VALUES (1, 1)")));

    // The cascade is what the tag table depends on for cleanup.
    QVERIFY(db.exec(QStringLiteral("DELETE FROM parent WHERE id = 1")));
    QCOMPARE(db.scalar(QStringLiteral("SELECT count(*) FROM child")), 0LL);
}

void TestDatabase::memoryDatabaseSkipsFilePragmas()
{
    // WAL is meaningless in memory and setting it fails; open() must not treat
    // that as an error, because every test below uses :memory:.
    Database db;
    QVERIFY(db.open(QStringLiteral(":memory:")));
    QVERIFY(db.isOpen());
    QVERIFY(db.exec(QStringLiteral("CREATE TABLE t (a INTEGER)")));
}

void TestDatabase::transactionCommitPersists()
{
    Database db;
    QVERIFY(db.open(QStringLiteral(":memory:")));
    QVERIFY(db.exec(QStringLiteral("CREATE TABLE t (a INTEGER)")));

    {
        Transaction tx(db);
        QVERIFY(tx.isActive());
        QVERIFY(db.exec(QStringLiteral("INSERT INTO t (a) VALUES (1)")));
        QVERIFY(tx.commit());
    }

    QCOMPARE(db.scalar(QStringLiteral("SELECT count(*) FROM t")), 1LL);
}

void TestDatabase::transactionRollsBackWhenScopeExits()
{
    // The property that matters: an early return anywhere inside a write batch
    // leaves nothing behind. Nobody has to remember to roll back.
    Database db;
    QVERIFY(db.open(QStringLiteral(":memory:")));
    QVERIFY(db.exec(QStringLiteral("CREATE TABLE t (a INTEGER)")));

    {
        Transaction tx(db);
        QVERIFY(tx.isActive());
        QVERIFY(db.exec(QStringLiteral("INSERT INTO t (a) VALUES (1)")));
        // No commit — destructor rolls back.
    }

    QCOMPARE(db.scalar(QStringLiteral("SELECT count(*) FROM t")), 0LL);
    QVERIFY(!db.inTransaction());
}

void TestDatabase::transactionRollsBackOnExplicitCall()
{
    Database db;
    QVERIFY(db.open(QStringLiteral(":memory:")));
    QVERIFY(db.exec(QStringLiteral("CREATE TABLE t (a INTEGER)")));

    Transaction tx(db);
    QVERIFY(db.exec(QStringLiteral("INSERT INTO t (a) VALUES (1)")));
    tx.rollback();

    QVERIFY(!tx.isActive());
    QCOMPARE(db.scalar(QStringLiteral("SELECT count(*) FROM t")), 0LL);
}

void TestDatabase::nestedTransactionIsInertAndDoesNotCommitOuter()
{
    Database db;
    QVERIFY(db.open(QStringLiteral(":memory:")));
    QVERIFY(db.exec(QStringLiteral("CREATE TABLE t (a INTEGER)")));

    {
        Transaction outer(db);
        QVERIFY(outer.isActive());
        QVERIFY(db.exec(QStringLiteral("INSERT INTO t (a) VALUES (1)")));

        {
            QTest::ignoreMessage(QtWarningMsg,
                                 "catalog: nested transaction requested; inner scope is inert");
            Transaction inner(db);
            QVERIFY(!inner.isActive());
            QVERIFY(!inner.commit());
        }

        // The inner scope ending must not have committed or rolled back the
        // outer one — the write is still pending and still reversible.
        QVERIFY(db.inTransaction());
        QCOMPARE(db.scalar(QStringLiteral("SELECT count(*) FROM t")), 1LL);
    }

    QCOMPARE(db.scalar(QStringLiteral("SELECT count(*) FROM t")), 0LL);
}

void TestDatabase::execBatchIsAtomic()
{
    Database db;
    QVERIFY(db.open(QStringLiteral(":memory:")));

    QStringList statements;
    statements << QStringLiteral("CREATE TABLE a (x INTEGER)")
               << QStringLiteral("CREATE TABLE b (x INTEGER)")
               << QStringLiteral("THIS IS NOT SQL");

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("catalog: query failed")));
    QVERIFY(!db.execBatch(statements));

    // Neither table should exist: the batch rolled back as a unit.
    QCOMPARE(db.scalar(QStringLiteral("SELECT count(*) FROM sqlite_master WHERE type='table'")),
             0LL);
}

void TestDatabase::scalarReturnsFallbackOnFailure()
{
    Database db;
    QVERIFY(db.open(QStringLiteral(":memory:")));

    QCOMPARE(db.scalar(QStringLiteral("SELECT x FROM nonexistent"), -7), -7LL);
    QCOMPARE(db.scalar(QStringLiteral("SELECT NULL"), -7), -7LL);
    QCOMPARE(db.scalar(QStringLiteral("SELECT 42"), -7), 42LL);
}

void TestDatabase::readOnlyConnectionRejectsWrites()
{
    const QString path = dbFile(QStringLiteral("readonly.db"));

    {
        Database writable;
        QVERIFY(writable.open(path));
        QVERIFY(writable.exec(QStringLiteral("CREATE TABLE t (a INTEGER)")));
        QVERIFY(writable.exec(QStringLiteral("INSERT INTO t (a) VALUES (1)")));
    }

    Database::Options options;
    options.readOnly = true;

    Database readonly;
    QVERIFY(readonly.open(path, options));
    QCOMPARE(readonly.scalar(QStringLiteral("SELECT count(*) FROM t")), 1LL);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("catalog: query failed")));
    QVERIFY(!readonly.exec(QStringLiteral("INSERT INTO t (a) VALUES (2)")));
}

void TestDatabase::separateConnectionsCanReadConcurrently()
{
    // WAL's actual promise: a reader on one connection is not blocked by an
    // open write transaction on another. The reconciliation pass depends on
    // this, since it runs off-thread while the GUI reads.
    const QString path = dbFile(QStringLiteral("concurrent.db"));

    Database writer;
    QVERIFY(writer.open(path));
    QVERIFY(writer.exec(QStringLiteral("CREATE TABLE t (a INTEGER)")));
    QVERIFY(writer.exec(QStringLiteral("INSERT INTO t (a) VALUES (1)")));

    Database reader;
    QVERIFY(reader.open(path));

    Transaction tx(writer);
    QVERIFY(tx.isActive());
    QVERIFY(writer.exec(QStringLiteral("INSERT INTO t (a) VALUES (2)")));

    // Uncommitted write is invisible to the reader, and the read succeeds
    // rather than blocking until the busy timeout expires.
    QCOMPARE(reader.scalar(QStringLiteral("SELECT count(*) FROM t")), 1LL);

    QVERIFY(tx.commit());
    QCOMPARE(reader.scalar(QStringLiteral("SELECT count(*) FROM t")), 2LL);
}

QTEST_GUILESS_MAIN(TestDatabase)
#include "tst_database.moc"
