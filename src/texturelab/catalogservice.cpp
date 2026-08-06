#include "catalogservice.h"

#include "models.h"
#include "telemetry.h"

#include <QBuffer>
#include <QDateTime>
#include <QImage>
#include <QDir>
#include <QFileInfo>
#include <QRunnable>
#include <QSettings>
#include <QStandardPaths>
#include <QThreadPool>

namespace {

qint64 nowMs()
{
    return QDateTime::currentMSecsSinceEpoch();
}

// TextureChannel is the app's enum; ChannelBit is the catalog's. They are
// deliberately separate types — see texturerecord.h — so this is the one place
// that knows both. Adding a channel means adding it here and to ChannelBit,
// never renumbering the existing bits.
int channelBitFor(TextureChannel channel)
{
    switch (channel) {
    case TextureChannel::Albedo:
        return catalog::ChannelAlbedo;
    case TextureChannel::Normal:
        return catalog::ChannelNormal;
    case TextureChannel::Metalness:
        return catalog::ChannelMetalness;
    case TextureChannel::Roughness:
        return catalog::ChannelRoughness;
    case TextureChannel::Height:
        return catalog::ChannelHeight;
    case TextureChannel::Alpha:
        return catalog::ChannelAlpha;
    case TextureChannel::AO:
        return catalog::ChannelAO;
    case TextureChannel::None:
        break;
    }
    return catalog::ChannelNone;
}

// Walks every known path and records what's actually on disk.
//
// Opens its own database connections rather than borrowing the service's: a
// QSqlDatabase belongs to the thread that opened it, and sharing one across
// threads is the kind of bug that shows up as a corrupt read months later.
class ReconcileTask : public QRunnable {
public:
    ReconcileTask(CatalogService* owner, const QString& indexPath, const QString& thumbsPath)
        : service(owner), indexFile(indexPath), thumbsFile(thumbsPath)
    {
        setAutoDelete(true);
    }

    void run() override
    {
        catalog::CatalogIndex index;
        if (!index.open(indexFile)) {
            report(0, 0);
            return;
        }
        if (index.isReadOnly()) {
            report(0, 0);
            return;
        }

        catalog::ThumbnailCache thumbs;
        const bool haveThumbs = thumbs.open(thumbsFile);

        int missing = 0;
        int updated = 0;

        for (const catalog::TextureRecord& rec : index.allRecords()) {
            const QFileInfo info(rec.path);

            if (!info.exists()) {
                if (!rec.isMissing()) {
                    index.markMissing(rec.id, nowMs());
                    ++missing;
                }
                continue;
            }

            const qint64 size = info.size();
            const qint64 mtime = info.lastModified().toMSecsSinceEpoch();

            if (size == rec.fileSize && mtime == rec.fileMtime && !rec.isMissing())
                continue;

            // Size or mtime differ: the file was edited outside the app, so its
            // cached thumbnail shows a material that no longer exists. Dropping
            // it here is the whole reason this comparison exists — with no
            // content hash, nothing else would ever notice.
            if (haveThumbs && (size != rec.fileSize || mtime != rec.fileMtime))
                thumbs.removeTexture(rec.id);

            index.markPresent(rec.id, size, mtime);
            ++updated;
        }

        report(missing, updated);
    }

private:
    void report(int missing, int updated)
    {
        // Back to the GUI thread; the worker's connections are closed by the
        // time anyone reacts to this.
        QMetaObject::invokeMethod(
            service, [owner = service, missing, updated]() {
                owner->onReconcileFinished(missing, updated);
            },
            Qt::QueuedConnection);
    }

    CatalogService* service;
    QString indexFile;
    QString thumbsFile;
};

} // namespace

CatalogService& CatalogService::instance()
{
    // Heap-allocated and deliberately never deleted.
    //
    // A function-local static would be destroyed by __run_exit_handlers, which
    // runs *after* Qt has torn down its own globals. Closing a QSqlDatabase at
    // that point walks a driver registry whose lock has already been destroyed,
    // and the process segfaults on exit — after a clean run, which makes it look
    // like a crash on quit rather than a teardown-order bug.
    //
    // shutdown(), called from main() while Qt is still up, is the orderly path;
    // leaking this object is what guarantees no destructor runs later.
    static CatalogService* service = new CatalogService();
    return *service;
}

void CatalogService::shutdown()
{
    if (!ready)
        return;

    // An in-flight reconciliation holds its own connections and would otherwise
    // still be writing while we close ours.
    QThreadPool::globalInstance()->waitForDone(5000);

    thumbCache.close();
    catalogIndex.close();
    ready = false;
}

QString CatalogService::dataDir()
{
    // Matches what Telemetry already does for the crash database, rather than
    // the ~/.texturelab the spec sketched — one convention per app, and this
    // one is correct on Windows and macOS too.
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString CatalogService::indexPath()
{
    return dataDir() + QStringLiteral("/index.db");
}

QString CatalogService::thumbsPath()
{
    return dataDir() + QStringLiteral("/thumbs.db");
}

bool CatalogService::init()
{
    if (ready)
        return true;

    QDir().mkpath(dataDir());

    if (!catalogIndex.open(indexPath())) {
        qWarning("catalog: index unavailable (%s); launcher will be empty",
                 qPrintable(catalogIndex.lastError()));
        Telemetry::breadcrumb("catalog", "index open failed");
        return false;
    }

    // A cache that won't open is not a reason to fail: every read is allowed to
    // miss, and cards fall back to placeholders.
    if (!thumbCache.open(thumbsPath()))
        qWarning("catalog: thumbnail cache unavailable; cards will show placeholders");

    ready = true;

    if (!catalogIndex.isReadOnly())
        seedFromRecentFiles();

    Telemetry::breadcrumb("catalog", "opened index at " + indexPath().toStdString());
    return true;
}

void CatalogService::seedFromRecentFiles()
{
    // index.db can't be rebuilt by rescanning (LAUNCHER_PRD.md §1.1), so the
    // very first launch after this ships would otherwise show an empty window
    // to someone with a year of work on disk. The old QSettings recents list is
    // the one record of that history we already have.
    QSettings settings(QSettings::UserScope, "texturelab", "texturelab");
    if (settings.value("catalogSeeded", false).toBool())
        return;

    const QStringList files = settings.value("recentFiles").toStringList();
    const qint64 now = nowMs();
    int seeded = 0;

    // Ordered most-recent-first with no timestamps, so synthesize descending
    // ones a second apart. Preserving the order is the point; the absolute
    // values are meaningless and get overwritten on first real open.
    for (int i = 0; i < files.size(); ++i) {
        const QFileInfo info(files[i]);
        if (!info.exists())
            continue;

        catalog::TextureRecord rec;
        rec.path = info.absoluteFilePath();
        rec.name = info.completeBaseName();
        rec.fileSize = info.size();
        rec.fileMtime = info.lastModified().toMSecsSinceEpoch();

        // No metadata: reading it would mean parsing every file at startup, and
        // it fills itself in the first time each one is opened. Until then the
        // card shows a name and a date, which is what the recents menu showed.
        if (catalogIndex.recordOpened(rec, now - qint64(i) * 1000))
            ++seeded;
    }

    settings.setValue("catalogSeeded", true);

    if (seeded > 0) {
        qInfo("catalog: seeded %d texture(s) from the recent-files list", seeded);
        Telemetry::breadcrumb("catalog", "seeded " + std::to_string(seeded) + " from recents");
        emit catalogChanged();
    }
}

catalog::TextureRecord CatalogService::recordFor(const TextureProjectPtr& project,
                                                const QString& path)
{
    const QFileInfo info(path);

    catalog::TextureRecord rec;
    rec.path = info.absoluteFilePath();
    rec.name = info.completeBaseName();
    rec.fileSize = info.size();
    rec.fileMtime = info.lastModified().toMSecsSinceEpoch();

    if (!project)
        return rec;

    // Read from the live project rather than re-parsing the JSON: it's already
    // in memory at both write points, and it's the authority on anything the
    // file format doesn't store.
    rec.width = project->textureWidth;
    rec.height = project->textureHeight;
    rec.nodeCount = project->nodes.size();
    rec.libVersion = project->libraryVersion;

    int channels = catalog::ChannelNone;
    for (auto it = project->textureChannels.begin(); it != project->textureChannels.end(); ++it) {
        if (!it.value().isEmpty())
            channels |= channelBitFor(it.key());
    }
    rec.channels = channels;

    return rec;
}

qint64 CatalogService::recordOpened(const TextureProjectPtr& project, const QString& path)
{
    if (!ready || path.isEmpty())
        return -1;

    catalog::TextureRecord rec = recordFor(project, path);
    if (!catalogIndex.recordOpened(rec, nowMs())) {
        qWarning("catalog: could not record open of %s: %s", qPrintable(path),
                 qPrintable(catalogIndex.lastError()));
        return -1;
    }

    emit catalogChanged();
    return rec.id;
}

qint64 CatalogService::recordSaved(const TextureProjectPtr& project, const QString& path)
{
    if (!ready || path.isEmpty())
        return -1;

    catalog::TextureRecord rec = recordFor(project, path);
    if (!catalogIndex.recordSaved(rec, nowMs())) {
        qWarning("catalog: could not record save of %s: %s", qPrintable(path),
                 qPrintable(catalogIndex.lastError()));
        return -1;
    }

    // The file just changed, so any cached thumbnail is of the old material.
    // The caller captures a fresh one straight after this; dropping it here
    // means a failed capture leaves a placeholder rather than a stale picture.
    thumbCache.removeTexture(rec.id);

    emit catalogChanged();
    return rec.id;
}

void CatalogService::captureThumbnail(qint64 textureId, const QImage& frame,
                                      catalog::ThumbSource source)
{
    if (!ready || textureId < 0 || frame.isNull())
        return;

    const int side = qMin(frame.width(), frame.height());
    if (side < 32)
        return;

    // Center-crop to a square: the viewport is whatever shape the user left the
    // dock, and the cards are square. Cropping keeps the material at its own
    // scale, where squashing to fit would distort it.
    const QImage square = frame.copy((frame.width() - side) / 2, (frame.height() - side) / 2,
                                     side, side);

    QVector<catalog::ThumbnailCache::Entry> entries;
    for (int size : {512, 256}) {
        const QImage scaled =
            square.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        if (!scaled.save(&buffer, "JPG", 85))
            continue;

        catalog::ThumbnailCache::Entry entry;
        entry.key.textureId = textureId;
        entry.key.size = size;
        entry.bytes = bytes;
        entry.source = source;
        entries << entry;
    }

    if (entries.isEmpty())
        return;

    if (!thumbCache.putBatch(entries, nowMs())) {
        qWarning("catalog: could not cache thumbnail: %s", qPrintable(thumbCache.lastError()));
        return;
    }

    emit catalogChanged();
}

void CatalogService::forget(qint64 textureId)
{
    if (!ready || textureId < 0)
        return;

    thumbCache.removeTexture(textureId);
    if (catalogIndex.remove(textureId))
        emit catalogChanged();
}

void CatalogService::reconcileAsync()
{
    if (!ready || reconcileRunning)
        return;

    reconcileRunning = true;
    QThreadPool::globalInstance()->start(new ReconcileTask(this, indexPath(), thumbsPath()));
}

void CatalogService::onReconcileFinished(int missing, int updated)
{
    reconcileRunning = false;

    if (missing > 0 || updated > 0) {
        qInfo("catalog: reconcile marked %d missing, refreshed %d", missing, updated);
        emit catalogChanged();
    }

    emit reconcileFinished(missing, updated);
}
