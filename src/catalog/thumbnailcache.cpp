#include "thumbnailcache.h"

#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace catalog {

namespace {

const char* sourceToText(ThumbSource source)
{
    return source == ThumbSource::Save ? "save" : "open";
}

Database::Options cacheOptions()
{
    Database::Options options;
    // 8 KiB pages suit rows that are mostly a JPEG blob — fewer overflow pages
    // per image than the 4 KiB default. Both this and auto_vacuum only take
    // effect on an empty file, which is why the cache is recreated rather than
    // migrated when anything is wrong with it.
    options.pageSize = 8192;
    options.incrementalAutoVacuum = true;
    options.walMode = true;
    options.foreignKeys = false;
    return options;
}

} // namespace

ThumbnailCache::ThumbnailCache() = default;

ThumbnailCache::~ThumbnailCache()
{
    close();
}

bool ThumbnailCache::open(const QString& path)
{
    close();

    if (!db.open(path, cacheOptions()))
        return recreate(path);

    const bool fresh = db.scalar(
                           QStringLiteral("SELECT count(*) FROM sqlite_master WHERE type='table'"))
                       == 0;

    if (fresh)
        return createSchema();

    if (readSchemaVersion() != SchemaVersion) {
        qInfo("catalog: thumbnail cache schema mismatch; rebuilding %s", qPrintable(path));
        return recreate(path);
    }

    return true;
}

bool ThumbnailCache::recreate(const QString& path)
{
    db.close();

    // A cache has no history worth saving, so anything unexpected — a corrupt
    // file, an unreadable one, a schema from another build — is resolved by
    // starting over. Delete the WAL sidecars too, or SQLite will try to replay
    // them into the new file.
    QFile::remove(path);
    QFile::remove(path + QStringLiteral("-wal"));
    QFile::remove(path + QStringLiteral("-shm"));

    if (!db.open(path, cacheOptions()))
        return false;

    return createSchema();
}

bool ThumbnailCache::createSchema()
{
    QStringList statements;

    statements << QStringLiteral(R"(
        CREATE TABLE thumb (
          texture_id   INTEGER NOT NULL,
          mesh         TEXT    NOT NULL DEFAULT 'default',
          hdri         TEXT    NOT NULL DEFAULT 'default',
          size         INTEGER NOT NULL,
          format       TEXT    NOT NULL DEFAULT 'jpg',
          source       TEXT    NOT NULL,
          bytes        BLOB    NOT NULL,
          created_at   INTEGER NOT NULL,
          last_used    INTEGER NOT NULL,
          PRIMARY KEY (texture_id, mesh, hdri, size)
        ) WITHOUT ROWID
    )");

    statements << QStringLiteral("CREATE INDEX ix_evict ON thumb(last_used)");
    statements << QStringLiteral("CREATE TABLE meta (k TEXT PRIMARY KEY, v TEXT)");
    statements << QStringLiteral("INSERT INTO meta (k, v) VALUES ('schema_version', '%1')")
                      .arg(SchemaVersion);

    return db.execBatch(statements);
}

int ThumbnailCache::readSchemaVersion()
{
    QSqlQuery query = db.prepare(QStringLiteral("SELECT v FROM meta WHERE k = 'schema_version'"));
    if (!query.exec() || !query.next())
        return 0;

    return query.value(0).toInt();
}

void ThumbnailCache::close()
{
    db.close();
}

bool ThumbnailCache::put(const ThumbKey& key, const QByteArray& bytes, ThumbSource source,
                         qint64 whenMs)
{
    Entry entry;
    entry.key = key;
    entry.bytes = bytes;
    entry.source = source;
    return putBatch({entry}, whenMs);
}

bool ThumbnailCache::putBatch(const QVector<Entry>& entries, qint64 whenMs)
{
    if (entries.isEmpty())
        return true;

    Transaction tx(db);
    if (!tx.isActive())
        return false;

    for (const Entry& entry : entries) {
        if (!entry.key.isValid() || entry.bytes.isEmpty()) {
            db.setError(QStringLiteral("refusing to cache an empty or unkeyed thumbnail"));
            return false;
        }

        // created_at is preserved on conflict so the row keeps its original
        // provenance; last_used moves forward because we just produced it.
        QSqlQuery query = db.prepare(QStringLiteral(R"(
            INSERT INTO thumb (texture_id, mesh, hdri, size, format, source,
                               bytes, created_at, last_used)
            VALUES (?, ?, ?, ?, 'jpg', ?, ?, ?, ?)
            ON CONFLICT(texture_id, mesh, hdri, size) DO UPDATE SET
                source    = excluded.source,
                bytes     = excluded.bytes,
                last_used = excluded.last_used
        )"));

        query.addBindValue(entry.key.textureId);
        query.addBindValue(entry.key.mesh);
        query.addBindValue(entry.key.hdri);
        query.addBindValue(entry.key.size);
        query.addBindValue(QString::fromLatin1(sourceToText(entry.source)));
        query.addBindValue(entry.bytes);
        query.addBindValue(whenMs);
        query.addBindValue(whenMs);

        if (!query.exec()) {
            db.setError(query.lastError().text());
            qWarning("catalog: thumbnail write failed: %s", qPrintable(db.lastError()));
            return false;
        }
    }

    return tx.commit();
}

QByteArray ThumbnailCache::get(const ThumbKey& key) const
{
    if (!key.isValid())
        return QByteArray();

    QSqlQuery query = const_cast<Database&>(db).prepare(QStringLiteral(
        "SELECT bytes FROM thumb WHERE texture_id = ? AND mesh = ? AND hdri = ? AND size = ?"));
    query.addBindValue(key.textureId);
    query.addBindValue(key.mesh);
    query.addBindValue(key.hdri);
    query.addBindValue(key.size);

    if (!query.exec() || !query.next())
        return QByteArray();

    return query.value(0).toByteArray();
}

bool ThumbnailCache::contains(const ThumbKey& key) const
{
    if (!key.isValid())
        return false;

    QSqlQuery query = const_cast<Database&>(db).prepare(QStringLiteral(
        "SELECT 1 FROM thumb WHERE texture_id = ? AND mesh = ? AND hdri = ? AND size = ?"));
    query.addBindValue(key.textureId);
    query.addBindValue(key.mesh);
    query.addBindValue(key.hdri);
    query.addBindValue(key.size);

    return query.exec() && query.next();
}

bool ThumbnailCache::touch(const QVector<ThumbKey>& keys, qint64 whenMs)
{
    if (keys.isEmpty())
        return true;

    Transaction tx(db);
    if (!tx.isActive())
        return false;

    for (const ThumbKey& key : keys) {
        QSqlQuery query = db.prepare(QStringLiteral(
            "UPDATE thumb SET last_used = ? "
            "WHERE texture_id = ? AND mesh = ? AND hdri = ? AND size = ?"));
        query.addBindValue(whenMs);
        query.addBindValue(key.textureId);
        query.addBindValue(key.mesh);
        query.addBindValue(key.hdri);
        query.addBindValue(key.size);

        if (!query.exec()) {
            db.setError(query.lastError().text());
            return false;
        }
    }

    return tx.commit();
}

int ThumbnailCache::removeTexture(qint64 textureId)
{
    if (textureId < 0)
        return 0;

    QSqlQuery query = db.prepare(QStringLiteral("DELETE FROM thumb WHERE texture_id = ?"));
    query.addBindValue(textureId);

    if (!query.exec()) {
        db.setError(query.lastError().text());
        return 0;
    }
    return query.numRowsAffected();
}

qint64 ThumbnailCache::totalBytes() const
{
    return const_cast<Database&>(db).scalar(
        QStringLiteral("SELECT coalesce(sum(length(bytes)), 0) FROM thumb"));
}

int ThumbnailCache::rowCount() const
{
    return static_cast<int>(
        const_cast<Database&>(db).scalar(QStringLiteral("SELECT count(*) FROM thumb")));
}

int ThumbnailCache::evictTo(qint64 budgetBytes)
{
    qint64 total = totalBytes();
    if (total <= budgetBytes)
        return 0;

    // Walk oldest-first, deleting until we're under budget. Done in one
    // transaction so a crash mid-eviction can't leave a partially reaped cache
    // — not that it would matter much, but the free-page accounting below
    // assumes the deletes actually landed.
    QVector<QVariant> doomed;
    qint64 freed = 0;

    {
        QSqlQuery scan = db.prepare(QStringLiteral(
            "SELECT texture_id, mesh, hdri, size, length(bytes) FROM thumb "
            "ORDER BY last_used ASC"));
        if (!scan.exec()) {
            db.setError(scan.lastError().text());
            return 0;
        }

        while (scan.next() && (total - freed) > budgetBytes) {
            doomed << scan.value(0) << scan.value(1) << scan.value(2) << scan.value(3);
            freed += scan.value(4).toLongLong();
        }
    }

    if (doomed.isEmpty())
        return 0;

    Transaction tx(db);
    if (!tx.isActive())
        return 0;

    int deleted = 0;
    for (int i = 0; i + 3 < doomed.size(); i += 4) {
        QSqlQuery query = db.prepare(QStringLiteral(
            "DELETE FROM thumb WHERE texture_id = ? AND mesh = ? AND hdri = ? AND size = ?"));
        query.addBindValue(doomed[i]);
        query.addBindValue(doomed[i + 1]);
        query.addBindValue(doomed[i + 2]);
        query.addBindValue(doomed[i + 3]);

        if (!query.exec()) {
            db.setError(query.lastError().text());
            return 0;
        }
        deleted += query.numRowsAffected();
    }

    if (!tx.commit())
        return 0;

    // Hand the freed pages back to the filesystem. Incremental rather than a
    // full VACUUM so this stays bounded; auto_vacuum was set to INCREMENTAL at
    // creation precisely so this call works at all.
    db.exec(QStringLiteral("PRAGMA incremental_vacuum"));

    return deleted;
}

} // namespace catalog
