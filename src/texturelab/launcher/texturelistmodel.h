#pragma once

#include "texturerecord.h"

#include <QAbstractListModel>
#include <QHash>
#include <QPixmap>
#include <QVector>

namespace catalog {
class CatalogIndex;
class ThumbnailCache;
}

// The launcher's model over the catalog index.
//
// View-agnostic on purpose: it exposes typed roles and no formatting. The grid
// delegate and the eventual list delegate both read the same roles, and neither
// the "2h ago" string nor the "4K" string is built here — a model that formats
// is a model that can only feed one view.
//
// Rows are paged in with canFetchMore()/fetchMore() rather than loaded whole, so
// first paint costs one small query no matter how much history has accumulated.
class TextureListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        PathRole,
        ModifiedRole,   // qint64 ms — file mtime
        OpenedRole,     // qint64 ms, 0 if never
        SavedRole,      // qint64 ms, 0 if never
        FileSizeRole,   // qint64 bytes
        WidthRole,
        HeightRole,
        NodeCountRole,
        ChannelsRole,   // catalog::ChannelBit mask
        LibVersionRole,
        StarredRole,
        MissingRole,
        NeedsMigrationRole,
        IsOpenRole,     // currently loaded in the editor
        ThumbnailRole,  // QPixmap; null when nothing has been captured yet
        RecordRole,     // the whole catalog::TextureRecord
    };

    // Rows fetched per batch. Comfortably more than fills a 1280×820 window, so
    // the first screen never waits on a second query.
    static constexpr int PageSize = 200;

    explicit TextureListModel(QObject* parent = nullptr);

    // The index is borrowed, not owned, and must outlive the model.
    void setIndex(catalog::CatalogIndex* index);

    // Optional. Without it every card paints a placeholder, which is the
    // correct degraded state when the cache can't be opened.
    void setThumbnailCache(catalog::ThumbnailCache* cache);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // Total matching rows, including any not yet fetched. The empty-state and
    // result-count copy needs this, not rowCount().
    int totalCount() const { return total; }

    catalog::Filter filter() const { return query.filter; }
    void setFilter(catalog::Filter filter);

    void setSort(catalog::SortKey key, bool ascending);
    catalog::SortKey sortKey() const { return query.sort; }
    bool sortAscending() const { return query.ascending; }

    QString searchTerm() const { return query.search; }
    void setSearchTerm(const QString& term);

    // Marks which texture is loaded in the editor, for the "currently open"
    // indicator. Exactly one, since the app is single-document.
    void setOpenPath(const QString& path);

    // The library version rows are compared against for the migration badge.
    // Injected rather than read from libraries/libversion.h so this model
    // depends on nothing but Qt and the catalog, and can be tested headless.
    void setCurrentLibVersion(const QString& version);

    catalog::TextureRecord recordAt(const QModelIndex& index) const;
    QModelIndex indexForId(qint64 id) const;

public slots:
    // Re-runs the query from scratch, keeping the first page's worth of rows.
    // Called whenever the catalog changes underneath us.
    void refresh();

private:
    void reload();

    catalog::CatalogIndex* catalogIndex = nullptr;
    catalog::Query query;
    QVector<catalog::TextureRecord> rows;

    // Decoded pixmaps, keyed by texture id. JPEG decoding during scroll would
    // be visible, and the same card is repainted constantly — on hover, on
    // selection, on every scroll pixel.
    mutable QHash<qint64, QPixmap> pixmaps;

    catalog::ThumbnailCache* thumbnails = nullptr;
    QString openPath;
    QString currentVersion;
    int total = 0;
};
