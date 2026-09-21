#pragma once

#include "database.h"
#include "texturerecord.h"

#include <QStringList>
#include <QVector>

namespace catalog {

// Repository over index.db — the durable record of every texture the app has
// created, opened, or saved.
//
// There is no filesystem scanner. Rows appear here only because the user did
// something, which means this file is the *only* record of what the launcher
// knows; losing it loses history even though every .texture file is still on
// disk. Two consequences are baked into this class: nothing is ever deleted
// implicitly (missing files are flagged, not removed), and a database written
// by a newer build is opened read-only rather than migrated speculatively.
class CatalogIndex {
public:
    // Bump when the schema changes, and add a step to migrate().
    static constexpr int SchemaVersion = 1;

    CatalogIndex();
    ~CatalogIndex();

    CatalogIndex(const CatalogIndex&) = delete;
    CatalogIndex& operator=(const CatalogIndex&) = delete;

    // Creates the schema if the file is new, migrates it if it's older, and
    // falls back to read-only if it's newer than this build understands.
    bool open(const QString& path);
    void close();

    bool isOpen() const { return db.isOpen(); }

    // True when the file was written by a newer build. Every mutating call
    // fails in this state; the launcher should still show what it can.
    bool isReadOnly() const { return readOnly; }

    int schemaVersion() const { return currentVersion; }
    QString lastError() const { return db.lastError(); }
    Database& database() { return db; }

    // --- write points (see LAUNCHER_PRD.md §6.1) -------------------------

    // Upserts by path and stamps last_opened. On success `rec.id` is filled in.
    //
    // Paths are identity here. A texture moved on disk becomes a new row at its
    // new path, and the old one stays behind, flagged missing until the user
    // removes it — no attempt is made to recognize the two as the same file.
    // Detecting that needs a content hash or an mtime heuristic, and neither
    // earns its keep for how often textures actually move.
    bool recordOpened(TextureRecord& rec, qint64 whenMs);

    // Upserts by path and stamps last_saved.
    bool recordSaved(TextureRecord& rec, qint64 whenMs);

    bool setStarred(qint64 id, bool starred);

    // Forgets a texture. Never touches the file on disk.
    bool remove(qint64 id);
    int removeAllMissing();

    // Clears last_opened everywhere, emptying the Recents view. Keeps the rows,
    // their stars, and their tags — "clear recents" is about history, not about
    // discarding what the user has collected.
    bool clearRecents();

    // --- reconciliation (see LAUNCHER_PRD.md §6.2) -----------------------

    // Every row, cheapest form, for the stat() pass on launcher open.
    QVector<TextureRecord> allRecords() const;

    bool markMissing(qint64 id, qint64 whenMs);

    // Records that a file is present with this size/mtime, clearing any missing
    // flag. Metadata is refreshed on next open, not here — this pass must not
    // parse files.
    //
    // A caller that sees size or mtime differ from the stored row knows the
    // file was edited outside the app, and should drop that texture's cached
    // thumbnail before calling this. That comparison is the only change
    // detection in the system.
    bool markPresent(qint64 id, qint64 fileSize, qint64 fileMtime);

    // Re-points a row at a new path — the user found a file that had gone
    // missing. This is the deliberate counterpart to not detecting moves
    // automatically (§6.2): the launcher can't guess, but the user can tell it.
    //
    // If another row already occupies `newPath`, the two are merged: the
    // survivor keeps that path and inherits stars, tags, and the earlier
    // created_at, and the relocated row is deleted. Returns the surviving row
    // id, or -1 on failure.
    qint64 relocate(qint64 id, const QString& newPath, qint64 fileSize, qint64 fileMtime);

    // --- reads ------------------------------------------------------------

    QVector<TextureRecord> list(const Query& query) const;
    int count(const Query& query) const;

    TextureRecord byId(qint64 id) const;
    TextureRecord byPath(const QString& path) const;

    // --- tags -------------------------------------------------------------

    QStringList tags(qint64 id) const;
    bool addTag(qint64 id, const QString& tag);
    bool removeTag(qint64 id, const QString& tag);

private:
    enum class Stamp { Opened, Saved };

    bool createSchema();
    bool migrate(int fromVersion);
    int readSchemaVersion();
    bool writeSchemaVersion(int version);

    bool upsert(TextureRecord& rec, Stamp stamp, qint64 whenMs);

    static TextureRecord readRow(const class QSqlQuery& query);
    static QString orderByClause(const Query& query);
    static QString whereClause(const Query& query);

    Database db;
    bool readOnly = false;
    int currentVersion = 0;
};

} // namespace catalog
