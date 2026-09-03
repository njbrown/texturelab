#include "texturelistmodel.h"

#include "catalogindex.h"
#include "thumbnailcache.h"

#include <QVariant>

TextureListModel::TextureListModel(QObject* parent) : QAbstractListModel(parent)
{
    query.filter = catalog::Filter::All;
    query.sort = catalog::SortKey::Modified;
    query.ascending = false;
    query.limit = PageSize;
    query.offset = 0;
}

void TextureListModel::setIndex(catalog::CatalogIndex* index)
{
    catalogIndex = index;
    reload();
}

void TextureListModel::setThumbnailCache(catalog::ThumbnailCache* cache)
{
    thumbnails = cache;
    pixmaps.clear();

    if (!rows.isEmpty())
        emit dataChanged(index(0), index(rows.size() - 1), {ThumbnailRole});
}

void TextureListModel::reload()
{
    beginResetModel();
    rows.clear();
    // Dropped wholesale rather than selectively: a reload follows a save or a
    // reconcile, either of which may have replaced any card's image.
    pixmaps.clear();
    total = 0;

    if (catalogIndex && catalogIndex->isOpen()) {
        catalog::Query page = query;
        page.limit = PageSize;
        page.offset = 0;
        rows = catalogIndex->list(page);
        total = catalogIndex->count(query);
    }

    endResetModel();
}

void TextureListModel::refresh()
{
    // Deliberately a full reset rather than a diff. The alternative is
    // reconciling two ordered sets to emit precise row moves, which is a real
    // source of off-by-one crashes, and the launcher only refreshes on discrete
    // user actions — never mid-scroll.
    reload();
}

int TextureListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return rows.size();
}

bool TextureListModel::canFetchMore(const QModelIndex& parent) const
{
    if (parent.isValid())
        return false;
    return rows.size() < total;
}

void TextureListModel::fetchMore(const QModelIndex& parent)
{
    if (parent.isValid() || !catalogIndex || !catalogIndex->isOpen())
        return;

    catalog::Query page = query;
    page.limit = PageSize;
    page.offset = rows.size();

    const QVector<catalog::TextureRecord> more = catalogIndex->list(page);
    if (more.isEmpty()) {
        // The table shrank under us (rows removed elsewhere). Trust what we
        // actually have rather than looping on a stale total.
        total = rows.size();
        return;
    }

    beginInsertRows(QModelIndex(), rows.size(), rows.size() + more.size() - 1);
    rows += more;
    endInsertRows();
}

void TextureListModel::setFilter(catalog::Filter filter)
{
    if (query.filter == filter)
        return;
    query.filter = filter;
    reload();
}

void TextureListModel::setSort(catalog::SortKey key, bool ascending)
{
    if (query.sort == key && query.ascending == ascending)
        return;
    query.sort = key;
    query.ascending = ascending;
    reload();
}

void TextureListModel::setSearchTerm(const QString& term)
{
    if (query.search == term)
        return;
    query.search = term;
    reload();
}

void TextureListModel::setOpenPath(const QString& path)
{
    if (openPath == path)
        return;

    openPath = path;
    if (!rows.isEmpty())
        emit dataChanged(index(0), index(rows.size() - 1), {IsOpenRole});
}

void TextureListModel::setCurrentLibVersion(const QString& version)
{
    if (currentVersion == version)
        return;

    currentVersion = version;
    if (!rows.isEmpty())
        emit dataChanged(index(0), index(rows.size() - 1), {NeedsMigrationRole});
}

catalog::TextureRecord TextureListModel::recordAt(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rows.size())
        return catalog::TextureRecord();
    return rows.at(index.row());
}

QModelIndex TextureListModel::indexForId(qint64 id) const
{
    for (int row = 0; row < rows.size(); ++row) {
        if (rows.at(row).id == id)
            return index(row);
    }
    return QModelIndex();
}

QVariant TextureListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rows.size())
        return QVariant();

    const catalog::TextureRecord& rec = rows.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return rec.name;
    case Qt::ToolTipRole:
        // With no folder tree, the tooltip is where location lives.
        return rec.path;
    case IdRole:
        return rec.id;
    case PathRole:
        return rec.path;
    case ModifiedRole:
        return rec.fileMtime;
    case OpenedRole:
        return rec.lastOpened;
    case SavedRole:
        return rec.lastSaved;
    case FileSizeRole:
        return rec.fileSize;
    case WidthRole:
        return rec.width;
    case HeightRole:
        return rec.height;
    case NodeCountRole:
        return rec.nodeCount;
    case ChannelsRole:
        return rec.channels;
    case LibVersionRole:
        return rec.libVersion;
    case StarredRole:
        return rec.starred;
    case MissingRole:
        return rec.isMissing();
    case NeedsMigrationRole:
        // Seeded rows have no version recorded yet — don't badge those as
        // outdated when we simply haven't looked inside the file. Same when the
        // caller never told us what "current" is.
        if (rec.libVersion.isEmpty() || currentVersion.isEmpty())
            return false;
        return rec.libVersion.compare(currentVersion, Qt::CaseInsensitive) != 0;
    case IsOpenRole:
        return !openPath.isEmpty() && rec.path == openPath;
    case ThumbnailRole: {
        auto cached = pixmaps.constFind(rec.id);
        if (cached != pixmaps.constEnd())
            return *cached;

        QPixmap pixmap;
        if (thumbnails && thumbnails->isOpen()) {
            catalog::ThumbKey key;
            key.textureId = rec.id;
            key.size = 256;
            const QByteArray bytes = thumbnails->get(key);
            if (!bytes.isEmpty())
                pixmap.loadFromData(bytes, "JPG");
        }

        // A null pixmap is cached too — it means "asked and there wasn't one",
        // and without it every repaint re-queries the database for a miss.
        pixmaps.insert(rec.id, pixmap);
        return pixmap;
    }
    case RecordRole:
        return QVariant::fromValue(rec);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TextureListModel::roleNames() const
{
    QHash<int, QByteArray> names = QAbstractListModel::roleNames();
    names[IdRole] = "textureId";
    names[NameRole] = "name";
    names[PathRole] = "path";
    names[ModifiedRole] = "modified";
    names[OpenedRole] = "opened";
    names[SavedRole] = "saved";
    names[FileSizeRole] = "fileSize";
    names[WidthRole] = "width";
    names[HeightRole] = "height";
    names[NodeCountRole] = "nodeCount";
    names[ChannelsRole] = "channels";
    names[LibVersionRole] = "libVersion";
    names[StarredRole] = "starred";
    names[MissingRole] = "missing";
    names[NeedsMigrationRole] = "needsMigration";
    names[IsOpenRole] = "isOpen";
    names[ThumbnailRole] = "thumbnail";
    return names;
}
