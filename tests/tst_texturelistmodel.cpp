#include "catalogindex.h"
#include "texturelistmodel.h"

#include <QAbstractItemModelTester>
#include <QTemporaryDir>
#include <QtTest>

using namespace catalog;

namespace {

constexpr qint64 kT0 = 1'700'000'000'000LL;

TextureRecord makeRecord(const QString& name, int ordinal)
{
    TextureRecord rec;
    rec.path = QStringLiteral("/tex/%1.texture").arg(name);
    rec.name = name;
    rec.fileSize = 1000 * (ordinal + 1);
    rec.fileMtime = kT0 + qint64(ordinal) * 1000;
    rec.width = 2048;
    rec.height = 2048;
    rec.nodeCount = 12;
    rec.libVersion = QStringLiteral("v3");
    rec.channels = ChannelAlbedo | ChannelNormal;
    return rec;
}

} // namespace

class TestTextureListModel : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void emptyWithoutIndex();
    void passesModelTester();

    void exposesRolesWithoutFormatting();
    void reportsTotalSeparatelyFromRowCount();
    void pagesInWithFetchMore();

    void filterSwitchesRowSet();
    void sortChangesOrder();
    void searchNarrowsRows();

    void openPathDrivesIsOpenRole();
    void migrationBadgeOnlyForKnownOlderVersions();

    void refreshPicksUpExternalChanges();

private:
    void seed(int count);

    QScopedPointer<QTemporaryDir> dir;
    QScopedPointer<CatalogIndex> index;
    QScopedPointer<TextureListModel> model;
};

void TestTextureListModel::init()
{
    dir.reset(new QTemporaryDir);
    QVERIFY(dir->isValid());

    index.reset(new CatalogIndex);
    QVERIFY(index->open(dir->filePath(QStringLiteral("index.db"))));

    model.reset(new TextureListModel);
}

void TestTextureListModel::cleanup()
{
    model.reset();
    index.reset();
    dir.reset();
}

void TestTextureListModel::seed(int count)
{
    for (int i = 0; i < count; ++i) {
        TextureRecord rec = makeRecord(QStringLiteral("tex%1").arg(i, 4, 10, QLatin1Char('0')), i);
        QVERIFY(index->recordOpened(rec, kT0 + qint64(i) * 1000));
    }
}

void TestTextureListModel::emptyWithoutIndex()
{
    // The launcher builds its model before the catalog is necessarily open;
    // that must be an empty grid, not a crash.
    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(model->totalCount(), 0);
    QVERIFY(!model->data(model->index(0), TextureListModel::NameRole).isValid());
    QVERIFY(!model->canFetchMore(QModelIndex()));
}

void TestTextureListModel::passesModelTester()
{
    // Catches the whole class of index/rowCount/parent contract violations that
    // otherwise surface as a view crash on someone's machine.
    seed(10);
    QAbstractItemModelTester tester(model.data(), QAbstractItemModelTester::FailureReportingMode::Warning);
    model->setIndex(index.data());
    model->setFilter(Filter::Recents);
    model->setSort(SortKey::Name, true);
    model->setSearchTerm(QStringLiteral("tex000"));
    model->setSearchTerm(QString());
    model->refresh();
    QVERIFY(model->rowCount() > 0);
}

void TestTextureListModel::exposesRolesWithoutFormatting()
{
    seed(1);
    model->setIndex(index.data());
    QCOMPARE(model->rowCount(), 1);

    const QModelIndex idx = model->index(0);

    // Raw values, not display strings: no "2h ago", no "2K". Formatting belongs
    // to the delegate, so the grid and a future list view can differ.
    QCOMPARE(idx.data(TextureListModel::NameRole).toString(), QStringLiteral("tex0000"));
    QCOMPARE(idx.data(TextureListModel::ModifiedRole).toLongLong(), kT0);
    QCOMPARE(idx.data(TextureListModel::WidthRole).toInt(), 2048);
    QCOMPARE(idx.data(TextureListModel::HeightRole).toInt(), 2048);
    QCOMPARE(idx.data(TextureListModel::NodeCountRole).toInt(), 12);
    QCOMPARE(idx.data(TextureListModel::ChannelsRole).toInt(), int(ChannelAlbedo | ChannelNormal));
    QCOMPARE(idx.data(TextureListModel::StarredRole).toBool(), false);
    QCOMPARE(idx.data(TextureListModel::MissingRole).toBool(), false);
    QCOMPARE(idx.data(Qt::ToolTipRole).toString(), QStringLiteral("/tex/tex0000.texture"));

    const TextureRecord rec = model->recordAt(idx);
    QVERIFY(rec.isValid());
    QCOMPARE(model->indexForId(rec.id).row(), 0);
}

void TestTextureListModel::reportsTotalSeparatelyFromRowCount()
{
    // The empty-state copy keys off totalCount(), which must describe the whole
    // result set — not just the page that happens to be resident.
    seed(TextureListModel::PageSize + 25);
    model->setIndex(index.data());

    QCOMPARE(model->rowCount(), TextureListModel::PageSize);
    QCOMPARE(model->totalCount(), TextureListModel::PageSize + 25);
}

void TestTextureListModel::pagesInWithFetchMore()
{
    const int extra = 25;
    seed(TextureListModel::PageSize + extra);
    model->setIndex(index.data());

    QVERIFY(model->canFetchMore(QModelIndex()));

    QSignalSpy inserted(model.data(), &QAbstractItemModel::rowsInserted);
    model->fetchMore(QModelIndex());

    QCOMPARE(inserted.count(), 1);
    QCOMPARE(model->rowCount(), TextureListModel::PageSize + extra);
    QVERIFY(!model->canFetchMore(QModelIndex()));

    // Every row is distinct — an off-by-one in the offset would duplicate the
    // page boundary, which looks like a rendering glitch rather than a bug.
    QSet<QString> paths;
    for (int row = 0; row < model->rowCount(); ++row)
        paths.insert(model->index(row).data(TextureListModel::PathRole).toString());
    QCOMPARE(paths.size(), model->rowCount());
}

void TestTextureListModel::filterSwitchesRowSet()
{
    seed(3);

    // One saved but never opened, one starred.
    TextureRecord savedOnly = makeRecord(QStringLiteral("savedonly"), 99);
    QVERIFY(index->recordSaved(savedOnly, kT0));
    QVERIFY(index->setStarred(index->byPath(QStringLiteral("/tex/tex0000.texture")).id, true));

    model->setIndex(index.data());
    QCOMPARE(model->totalCount(), 4);

    model->setFilter(Filter::Recents);
    QCOMPARE(model->totalCount(), 3); // the save-only row is not a "recent"

    model->setFilter(Filter::Starred);
    QCOMPARE(model->totalCount(), 1);

    model->setFilter(Filter::All);
    QCOMPARE(model->totalCount(), 4);
}

void TestTextureListModel::sortChangesOrder()
{
    seed(4);
    model->setIndex(index.data());

    model->setSort(SortKey::Name, true);
    QCOMPARE(model->index(0).data(TextureListModel::NameRole).toString(),
             QStringLiteral("tex0000"));

    model->setSort(SortKey::Name, false);
    QCOMPARE(model->index(0).data(TextureListModel::NameRole).toString(),
             QStringLiteral("tex0003"));

    model->setSort(SortKey::Modified, false);
    QCOMPARE(model->index(0).data(TextureListModel::ModifiedRole).toLongLong(), kT0 + 3000);

    model->setSort(SortKey::Size, false);
    QCOMPARE(model->index(0).data(TextureListModel::FileSizeRole).toLongLong(), 4000LL);
}

void TestTextureListModel::searchNarrowsRows()
{
    seed(3);
    model->setIndex(index.data());
    QCOMPARE(model->totalCount(), 3);

    model->setSearchTerm(QStringLiteral("tex0001"));
    QCOMPARE(model->totalCount(), 1);
    QCOMPARE(model->rowCount(), 1);

    model->setSearchTerm(QString());
    QCOMPARE(model->totalCount(), 3);
}

void TestTextureListModel::openPathDrivesIsOpenRole()
{
    seed(2);
    model->setIndex(index.data());
    model->setSort(SortKey::Name, true);

    QVERIFY(!model->index(0).data(TextureListModel::IsOpenRole).toBool());

    QSignalSpy changed(model.data(), &QAbstractItemModel::dataChanged);
    model->setOpenPath(QStringLiteral("/tex/tex0000.texture"));

    QCOMPARE(changed.count(), 1);
    QVERIFY(model->index(0).data(TextureListModel::IsOpenRole).toBool());
    QVERIFY(!model->index(1).data(TextureListModel::IsOpenRole).toBool());
}

void TestTextureListModel::migrationBadgeOnlyForKnownOlderVersions()
{
    TextureRecord current = makeRecord(QStringLiteral("current"), 0);
    current.libVersion = QStringLiteral("v3");
    QVERIFY(index->recordOpened(current, kT0));

    TextureRecord old = makeRecord(QStringLiteral("old"), 1);
    old.libVersion = QStringLiteral("v1");
    QVERIFY(index->recordOpened(old, kT0));

    // Seeded from the recents list: we've never looked inside it.
    TextureRecord unknown = makeRecord(QStringLiteral("unknown"), 2);
    unknown.libVersion.clear();
    QVERIFY(index->recordOpened(unknown, kT0));

    model->setIndex(index.data());
    model->setCurrentLibVersion(QStringLiteral("v3"));
    model->setSort(SortKey::Name, true);

    auto badge = [this](int row) {
        return model->index(row).data(TextureListModel::NeedsMigrationRole).toBool();
    };

    QVERIFY(!badge(0)); // "current"
    QVERIFY(badge(1));  // "old"
    QVERIFY(!badge(2)); // "unknown" — absence of data is not evidence of age
}

void TestTextureListModel::refreshPicksUpExternalChanges()
{
    seed(2);
    model->setIndex(index.data());
    QCOMPARE(model->totalCount(), 2);

    TextureRecord added = makeRecord(QStringLiteral("added"), 5);
    QVERIFY(index->recordOpened(added, kT0 + 99'000));

    // The model doesn't watch the database; CatalogService::catalogChanged is
    // what drives this in the app.
    QCOMPARE(model->totalCount(), 2);

    model->refresh();
    QCOMPARE(model->totalCount(), 3);
}

QTEST_MAIN(TestTextureListModel)
#include "tst_texturelistmodel.moc"
