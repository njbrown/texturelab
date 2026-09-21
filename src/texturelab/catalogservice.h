#pragma once

#include "catalogindex.h"
#include "thumbnailcache.h"

#include <QImage>
#include <QObject>
#include <QSharedPointer>
#include <QString>

class TextureProject;
typedef QSharedPointer<TextureProject> TextureProjectPtr;

// The boundary between the app and the launcher's data layer.
//
// src/catalog/ knows nothing about the node graph on purpose, so everything
// that has to understand a TextureProject — resolution, node count, which
// output channels are wired up, which library version — lives here. Keeping the
// translation in one place means the persisted schema can't drift just because
// the graph model changed.
//
// Owns the process's writable connections to both databases. The reconciliation
// pass opens its own on a worker thread; nothing else may touch these off the
// GUI thread.
class CatalogService : public QObject {
    Q_OBJECT

public:
    static CatalogService& instance();

    // Opens both databases, creating the data directory if needed, and seeds
    // the index from the legacy recent-files list on first run. Safe to call
    // more than once. Returns false if the index could not be opened — the app
    // must still run in that case, just without a launcher.
    bool init();
    bool isReady() const { return ready; }

    // Closes both databases while Qt is still alive. Must be called from main()
    // after the event loop returns — see the comment on instance().
    void shutdown();

    static QString dataDir();
    static QString indexPath();
    static QString thumbsPath();

    catalog::CatalogIndex& index() { return catalogIndex; }
    catalog::ThumbnailCache& thumbnails() { return thumbCache; }

    // --- write points (LAUNCHER_PRD.md §6.1) ------------------------------

    // Both return the index row id, or -1 if the write failed.
    qint64 recordOpened(const TextureProjectPtr& project, const QString& path);
    qint64 recordSaved(const TextureProjectPtr& project, const QString& path);

    // Stores a thumbnail captured from the 3D viewport (LAUNCHER_PRD.md §4).
    // `frame` is the raw grab; it gets center-cropped to a square and written at
    // both cache sizes. A null or degenerate image is ignored rather than
    // caching a blank card.
    void captureThumbnail(qint64 textureId, const QImage& frame, catalog::ThumbSource source);

    // Forgets a texture and its thumbnails. Never touches the file on disk.
    void forget(qint64 textureId);

    // --- reconciliation (LAUNCHER_PRD.md §6.2) ----------------------------

    // stat()s every known path on a worker thread and updates missing/present
    // state. Cheap — the row count is bounded by the user's own activity — but
    // it runs off the GUI thread anyway, because one unmounted network path
    // will block it regardless of how few rows there are.
    void reconcileAsync();

    // Builds the index row for a live project. Public for testing and for
    // callers that want the metadata without writing it.
    static catalog::TextureRecord recordFor(const TextureProjectPtr& project, const QString& path);

    // Internal: called on the GUI thread by the reconciliation task when it
    // finishes. Not for general use.
    void onReconcileFinished(int missing, int updated);

signals:
    // Emitted on the GUI thread after any change to the index, so a launcher
    // model can refresh itself without polling.
    void catalogChanged();

    void reconcileFinished(int missing, int updated);

private:
    CatalogService() = default;

    void seedFromRecentFiles();

    catalog::CatalogIndex catalogIndex;
    catalog::ThumbnailCache thumbCache;
    bool ready = false;
    bool reconcileRunning = false;
};
