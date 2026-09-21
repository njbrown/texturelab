#pragma once

#include "database.h"

#include <QByteArray>
#include <QString>
#include <QVector>

namespace catalog {

// Identifies one cached image.
//
// Keyed by the index's texture id. An earlier draft keyed on a hash of the
// file's contents so that copies shared a thumbnail and edits invalidated
// themselves — but change detection turned out to be (file_size, file_mtime)
// from stat() either way, and the hash was only ever the key. Dropping it
// removed a dependency and a column for the price of one extra render the
// first time you open a copied file.
//
// The consequence is that this cache is coupled to index.db's row ids: delete
// the index and these rows are orphaned. That's acceptable precisely because
// the cache is disposable — clear it alongside.
struct ThumbKey {
    qint64 textureId = -1;
    QString mesh = QStringLiteral("default");
    QString hdri = QStringLiteral("default");
    int size = 256;

    bool isValid() const { return textureId >= 0 && size > 0; }
};

// Where a cached image came from. Lets a better capture supersede a cheaper one
// instead of the cache locking in whatever was written first.
enum class ThumbSource {
    Save, // captured from the 3D viewport at save time — the good one
    Open, // captured on open, for a file indexed before this feature existed
};

// Repository over thumbs.db — a pure cache.
//
// Deleting this file at any time must be harmless: it is rebuilt as the user
// saves and opens textures. That's why it lives apart from index.db, which
// cannot be rebuilt at all (see LAUNCHER_PRD.md §1.1), and why a schema
// mismatch here is handled by deleting the file rather than migrating it.
class ThumbnailCache {
public:
    static constexpr int SchemaVersion = 1;

    // Roughly one 512px and one 256px JPEG per texture, so this is generous.
    static constexpr qint64 DefaultBudgetBytes = 512LL * 1024 * 1024;

    ThumbnailCache();
    ~ThumbnailCache();

    ThumbnailCache(const ThumbnailCache&) = delete;
    ThumbnailCache& operator=(const ThumbnailCache&) = delete;

    // Recreates the file from scratch if it's missing, corrupt, or written to a
    // different schema version. Only returns false if even that fails.
    bool open(const QString& path);
    void close();

    bool isOpen() const { return db.isOpen(); }
    QString lastError() const { return db.lastError(); }

    bool put(const ThumbKey& key, const QByteArray& bytes, ThumbSource source, qint64 whenMs);

    // Writes many images in one transaction. One transaction per thumbnail
    // means one fsync per thumbnail, which is what makes a bulk write crawl.
    struct Entry {
        ThumbKey key;
        QByteArray bytes;
        ThumbSource source = ThumbSource::Save;
    };
    bool putBatch(const QVector<Entry>& entries, qint64 whenMs);

    // Returns an empty QByteArray on a miss. Deliberately does not update
    // last_used: this runs during scroll, and a write per painted card is
    // exactly what the "no disk writes on the GUI thread" rule forbids. Call
    // touch() later with what was actually used.
    QByteArray get(const ThumbKey& key) const;

    bool contains(const ThumbKey& key) const;

    // Batched last_used bookkeeping, flushed on idle or close. Day-granularity
    // timestamps are plenty for an LRU whose eviction budget is half a gigabyte.
    bool touch(const QVector<ThumbKey>& keys, qint64 whenMs);

    // Drops every variant of one texture. Called when reconciliation sees a
    // file's size or mtime change — the cached image no longer shows what's on
    // disk — and when a texture is removed from the launcher.
    int removeTexture(qint64 textureId);

    qint64 totalBytes() const;
    int rowCount() const;

    // Deletes least-recently-used rows until the cache fits in `budgetBytes`,
    // then returns pages to the filesystem. Returns the number of rows deleted.
    int evictTo(qint64 budgetBytes = DefaultBudgetBytes);

private:
    bool createSchema();
    bool recreate(const QString& path);
    int readSchemaVersion();

    Database db;
};

} // namespace catalog
