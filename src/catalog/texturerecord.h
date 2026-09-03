#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>

namespace catalog {

// Output channel bits, mirroring TextureChannel in src/texturelab/models.h.
//
// Deliberately redeclared instead of including models.h: the catalog library
// must not depend on the app's node graph, so that it stays testable on its own
// and so that a change to the graph model can't quietly alter what's already
// stored in the index. The mapping between the two lives at the app boundary
// (Phase 2), and the ordinals below must not be renumbered — they're persisted.
enum ChannelBit : int {
    ChannelNone = 0,
    ChannelAlbedo = 1 << 1,
    ChannelNormal = 1 << 2,
    ChannelMetalness = 1 << 3,
    ChannelRoughness = 1 << 4,
    ChannelHeight = 1 << 5,
    ChannelAlpha = 1 << 6,
    ChannelAO = 1 << 7,
};

// One row of the texture table.
//
// Timestamps are Unix milliseconds. 0 means "unset" and is written to the
// database as NULL — the partial index on recents and every `IS NULL` filter
// depend on that distinction, so don't start storing a real 0.
//
// There is deliberately no content hash. Change detection is (file_size,
// file_mtime) from stat(), which is what actually decides whether a file was
// edited outside the app; a hash would only have been a cache key, and the
// cache keys on `id` instead.
struct TextureRecord {
    qint64 id = -1;
    QString path;
    QString name;
    qint64 fileSize = 0;
    qint64 fileMtime = 0;
    int width = 0;
    int height = 0;
    int nodeCount = 0;
    QString libVersion;
    int channels = ChannelNone;
    qint64 createdAt = 0;
    qint64 lastOpened = 0;
    qint64 lastSaved = 0;
    bool starred = false;
    qint64 missingSince = 0;

    bool isValid() const { return id >= 0; }
    bool isMissing() const { return missingSince != 0; }
    bool hasChannel(ChannelBit bit) const { return (channels & bit) != 0; }
};

// Which set of rows a query covers.
enum class Filter {
    All,      // everything, including missing files (they render dimmed)
    Recents,  // opened at least once, excluding missing — "jump back in"
    Starred,  // starred, including missing
};

enum class SortKey {
    Modified,   // file_mtime — what the card's relative time shows
    Opened,     // last_opened
    Name,
    Size,
};

struct Query {
    Filter filter = Filter::All;
    SortKey sort = SortKey::Modified;
    bool ascending = false;

    // Case-insensitive substring match against name and tags. Empty disables
    // the filter entirely rather than matching everything, so the common path
    // doesn't pay for the tag subquery.
    QString search;

    // limit < 0 means unbounded. The grid pages, so it normally doesn't.
    int limit = -1;
    int offset = 0;
};

} // namespace catalog
