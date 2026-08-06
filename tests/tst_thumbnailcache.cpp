#include "thumbnailcache.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QPixmap>
#include <QTemporaryDir>
#include <QtTest>

using namespace catalog;

namespace {

constexpr qint64 kT0 = 1'700'000'000'000LL;

// Stand-in for an encoded JPEG. Content doesn't matter, only that it round
// trips byte for byte — a BLOB column that mangles data would be silent.
QByteArray fakeImage(int sizeBytes, char fill = 'x')
{
    return QByteArray(sizeBytes, fill);
}

ThumbKey keyFor(qint64 textureId, int size = 256)
{
    ThumbKey key;
    key.textureId = textureId;
    key.size = size;
    return key;
}

} // namespace

class TestThumbnailCache : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void createsSchemaWithTunedPragmas();
    void putGetRoundTripsExactBytes();
    void realJpegSurvivesStoreAndDecode();
    void missReturnsEmpty();
    void sizeAndMeshArePartOfTheKey();
    void putOverwritesExistingVariant();
    void putRejectsEmptyOrUnkeyedImages();
    void putBatchWritesAll();

    void removeTextureDropsEveryVariant();

    void getDoesNotWriteLastUsed();
    void touchUpdatesLastUsed();

    void evictionRemovesLeastRecentlyUsedFirst();
    void evictionIsNoOpUnderBudget();

    void rebuildsWhenSchemaVersionDiffers();
    void rebuildsWhenFileIsCorrupt();
    void deletingFileWhileClosedIsRecoverable();

private:
    QString cachePath() const { return dir->filePath(QStringLiteral("thumbs.db")); }

    QScopedPointer<QTemporaryDir> dir;
    QScopedPointer<ThumbnailCache> cache;
};

void TestThumbnailCache::init()
{
    dir.reset(new QTemporaryDir);
    QVERIFY(dir->isValid());
    cache.reset(new ThumbnailCache);
    QVERIFY(cache->open(cachePath()));
}

void TestThumbnailCache::cleanup()
{
    cache.reset();
    dir.reset();
}

void TestThumbnailCache::createsSchemaWithTunedPragmas()
{
    QVERIFY(cache->isOpen());
    QCOMPARE(cache->rowCount(), 0);
    QCOMPARE(cache->totalBytes(), 0LL);

    // Both of these can only be set on an empty file, so if the schema were
    // ever created before the PRAGMAs they'd silently revert to the defaults
    // and incremental_vacuum would become a no-op.
    Database raw;
    QVERIFY(raw.open(cachePath()));
    QCOMPARE(raw.scalar(QStringLiteral("PRAGMA page_size")), 8192LL);
    QCOMPARE(raw.scalar(QStringLiteral("PRAGMA auto_vacuum")), 2LL);
}

void TestThumbnailCache::putGetRoundTripsExactBytes()
{
    const QByteArray image = fakeImage(4096, '\x1');
    const ThumbKey key = keyFor(1);

    QVERIFY(cache->put(key, image, ThumbSource::Save, kT0));
    QVERIFY(cache->contains(key));
    QCOMPARE(cache->get(key), image);
    QCOMPARE(cache->rowCount(), 1);
    QCOMPARE(cache->totalBytes(), 4096LL);
}

void TestThumbnailCache::realJpegSurvivesStoreAndDecode()
{
    // The capture path end to end minus the GL grab: encode a real image the
    // way CatalogService::captureThumbnail does, store it, then decode it back
    // the way the model does. A BLOB column that truncated or re-encoded would
    // show up as a garbled card, which is hard to attribute after the fact.
    QImage source(256, 256, QImage::Format_RGB32);
    for (int y = 0; y < source.height(); ++y)
        for (int x = 0; x < source.width(); ++x)
            source.setPixel(x, y, qRgb(x, y, (x ^ y) & 0xFF));

    QByteArray encoded;
    QBuffer buffer(&encoded);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(source.save(&buffer, "JPG", 85));
    QVERIFY(!encoded.isEmpty());

    const ThumbKey key = keyFor(7);
    QVERIFY(cache->put(key, encoded, ThumbSource::Save, kT0));

    const QByteArray fetched = cache->get(key);
    QCOMPARE(fetched, encoded);

    // QImage, not QPixmap: a pixmap needs a QGuiApplication and this suite runs
    // guiless. The decode path is what's under test either way.
    QImage back;
    QVERIFY(back.loadFromData(fetched, "JPG"));
    QCOMPARE(back.size(), QSize(256, 256));

    // Lossy, so compare structure rather than exact pixels: a black or
    // transposed image would fail this while surviving a byte comparison.
    QVERIFY(qAbs(qRed(back.pixel(200, 10)) - 200) < 24);
    QVERIFY(qAbs(qGreen(back.pixel(10, 200)) - 200) < 24);
}

void TestThumbnailCache::missReturnsEmpty()
{
    QVERIFY(cache->get(keyFor(2)).isEmpty());
    QVERIFY(!cache->contains(keyFor(2)));

    // An unkeyed request is a miss, not a crash.
    QVERIFY(cache->get(ThumbKey()).isEmpty());
    QVERIFY(!cache->contains(ThumbKey()));
}

void TestThumbnailCache::sizeAndMeshArePartOfTheKey()
{
    QVERIFY(cache->put(keyFor(1, 256), fakeImage(100, 'a'), ThumbSource::Save, kT0));
    QVERIFY(cache->put(keyFor(1, 512), fakeImage(200, 'b'), ThumbSource::Save, kT0));

    ThumbKey otherMesh = keyFor(1, 256);
    otherMesh.mesh = QStringLiteral("cube");
    QVERIFY(cache->put(otherMesh, fakeImage(300, 'c'), ThumbSource::Save, kT0));

    QCOMPARE(cache->rowCount(), 3);
    QCOMPARE(cache->get(keyFor(1, 256)).at(0), 'a');
    QCOMPARE(cache->get(keyFor(1, 512)).at(0), 'b');
    QCOMPARE(cache->get(otherMesh).at(0), 'c');
}

void TestThumbnailCache::putOverwritesExistingVariant()
{
    const ThumbKey key = keyFor(1);
    QVERIFY(cache->put(key, fakeImage(100, 'a'), ThumbSource::Open, kT0));
    QVERIFY(cache->put(key, fakeImage(200, 'b'), ThumbSource::Save, kT0 + 1000));

    // A better capture supersedes a cheaper one rather than adding a row.
    QCOMPARE(cache->rowCount(), 1);
    QCOMPARE(cache->get(key).size(), 200);
    QCOMPARE(cache->get(key).at(0), 'b');
}

void TestThumbnailCache::putRejectsEmptyOrUnkeyedImages()
{
    QVERIFY(!cache->put(keyFor(1), QByteArray(), ThumbSource::Save, kT0));
    QVERIFY(!cache->put(ThumbKey(), fakeImage(100), ThumbSource::Save, kT0));
    QCOMPARE(cache->rowCount(), 0);
}

void TestThumbnailCache::putBatchWritesAll()
{
    // One transaction per thumbnail means one fsync per thumbnail; the batch
    // path exists so a bulk write doesn't crawl.
    QVector<ThumbnailCache::Entry> entries;
    for (int i = 0; i < 25; ++i) {
        ThumbnailCache::Entry entry;
        entry.key = keyFor(i);
        entry.bytes = fakeImage(512);
        entries << entry;
    }

    QVERIFY(cache->putBatch(entries, kT0));
    QCOMPARE(cache->rowCount(), 25);
    QCOMPARE(cache->totalBytes(), 25LL * 512);
}

void TestThumbnailCache::removeTextureDropsEveryVariant()
{
    // How a stale thumbnail is invalidated now that there's no content hash:
    // reconciliation sees size or mtime differ and drops the texture's images
    // outright. Every variant goes, not just the size that happened to be on
    // screen.
    QVERIFY(cache->put(keyFor(1, 256), fakeImage(100), ThumbSource::Save, kT0));
    QVERIFY(cache->put(keyFor(1, 512), fakeImage(100), ThumbSource::Save, kT0));
    QVERIFY(cache->put(keyFor(2, 256), fakeImage(100), ThumbSource::Save, kT0));

    QCOMPARE(cache->removeTexture(1), 2);
    QCOMPARE(cache->rowCount(), 1);
    QVERIFY(cache->contains(keyFor(2, 256)));

    QCOMPARE(cache->removeTexture(-1), 0);
    QCOMPARE(cache->removeTexture(999), 0);
}

void TestThumbnailCache::getDoesNotWriteLastUsed()
{
    // get() runs during scroll. A write per painted card is exactly what the
    // "no disk writes on the GUI thread" rule forbids, so reads must be pure.
    const ThumbKey key = keyFor(1);
    QVERIFY(cache->put(key, fakeImage(100), ThumbSource::Save, kT0));

    Database raw;
    QVERIFY(raw.open(cachePath()));
    const qint64 before = raw.scalar(QStringLiteral("SELECT last_used FROM thumb"));

    for (int i = 0; i < 10; ++i)
        QVERIFY(!cache->get(key).isEmpty());

    QCOMPARE(raw.scalar(QStringLiteral("SELECT last_used FROM thumb")), before);
}

void TestThumbnailCache::touchUpdatesLastUsed()
{
    const ThumbKey key = keyFor(1);
    QVERIFY(cache->put(key, fakeImage(100), ThumbSource::Save, kT0));

    QVERIFY(cache->touch({key}, kT0 + 86'400'000));

    Database raw;
    QVERIFY(raw.open(cachePath()));
    QCOMPARE(raw.scalar(QStringLiteral("SELECT last_used FROM thumb")), kT0 + 86'400'000);

    QVERIFY(cache->touch({}, kT0)); // empty batch is fine
}

void TestThumbnailCache::evictionRemovesLeastRecentlyUsedFirst()
{
    // Ten 10 KiB images, each used a day apart.
    for (int i = 0; i < 10; ++i) {
        const ThumbKey key = keyFor(i);
        QVERIFY(cache->put(key, fakeImage(10 * 1024), ThumbSource::Save,
                           kT0 + qint64(i) * 86'400'000));
    }
    QCOMPARE(cache->totalBytes(), 10LL * 10 * 1024);

    // Trim to roughly half.
    const int deleted = cache->evictTo(50 * 1024);
    QCOMPARE(deleted, 5);
    QCOMPARE(cache->rowCount(), 5);
    QVERIFY(cache->totalBytes() <= 50 * 1024);

    // The five that survived are the five most recently used.
    for (int i = 0; i < 5; ++i)
        QVERIFY(!cache->contains(keyFor(i)));
    for (int i = 5; i < 10; ++i)
        QVERIFY(cache->contains(keyFor(i)));
}

void TestThumbnailCache::evictionIsNoOpUnderBudget()
{
    QVERIFY(cache->put(keyFor(1), fakeImage(1024), ThumbSource::Save, kT0));

    QCOMPARE(cache->evictTo(ThumbnailCache::DefaultBudgetBytes), 0);
    QCOMPARE(cache->rowCount(), 1);
}

void TestThumbnailCache::rebuildsWhenSchemaVersionDiffers()
{
    // A cache has no history worth migrating, so a schema from another build
    // is thrown away rather than upgraded. This is the difference that makes
    // thumbs.db disposable and index.db not.
    QVERIFY(cache->put(keyFor(1), fakeImage(1024), ThumbSource::Save, kT0));
    cache->close();

    {
        Database raw;
        QVERIFY(raw.open(cachePath()));
        QVERIFY(raw.exec(QStringLiteral("UPDATE meta SET v = '99' WHERE k = 'schema_version'")));
    }

    QVERIFY(cache->open(cachePath()));
    QVERIFY(cache->isOpen());
    QCOMPARE(cache->rowCount(), 0);

    // And it's usable immediately afterwards.
    QVERIFY(cache->put(keyFor(1), fakeImage(1024), ThumbSource::Save, kT0));
    QCOMPARE(cache->rowCount(), 1);
}

void TestThumbnailCache::rebuildsWhenFileIsCorrupt()
{
    cache->close();

    {
        QFile file(cachePath());
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("this is not a database, it is a picture of a database");
    }

    QVERIFY(cache->open(cachePath()));
    QVERIFY(cache->isOpen());
    QVERIFY(cache->put(keyFor(1), fakeImage(1024), ThumbSource::Save, kT0));
    QCOMPARE(cache->rowCount(), 1);
}

void TestThumbnailCache::deletingFileWhileClosedIsRecoverable()
{
    // "Deleting thumbs.db degrades gracefully" from the acceptance criteria.
    QVERIFY(cache->put(keyFor(1), fakeImage(1024), ThumbSource::Save, kT0));
    cache->close();

    QVERIFY(QFile::remove(cachePath()));

    QVERIFY(cache->open(cachePath()));
    QCOMPARE(cache->rowCount(), 0);
    QVERIFY(cache->get(keyFor(1)).isEmpty());
    QVERIFY(cache->put(keyFor(1), fakeImage(1024), ThumbSource::Save, kT0));
}

QTEST_GUILESS_MAIN(TestThumbnailCache)
#include "tst_thumbnailcache.moc"
