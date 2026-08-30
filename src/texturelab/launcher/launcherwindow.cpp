#include "launcherwindow.h"

#include "catalogservice.h"
#include "libraries/libversion.h"
#include "texturecarddelegate.h"
#include "texturelistmodel.h"
#include "texturerowdelegate.h"
#include "update/updatechecker.h"

#include <QButtonGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPair>
#include <QPushButton>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QSettings>
#include <QSlider>
#include <QStringList>
#include <QStyle>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

namespace {

// Object names are the hook for resources/qss/app.qss.in — the launcher adds no
// inline stylesheets (see scripts/check-theme-hygiene.sh).
constexpr const char* kTopBarName = "launcherTopBar";
constexpr const char* kActionBarName = "launcherActionBar";
constexpr const char* kGridName = "launcherGrid";
constexpr const char* kFilterTabName = "launcherFilterTab";
constexpr const char* kEmptyPanelName = "launcherEmptyPanel";
constexpr const char* kEmptyLabelName = "launcherEmptyLabel";
constexpr const char* kUpdateButtonName = "launcherUpdateButton";

bool isTextureFile(const QUrl& url)
{
    return url.isLocalFile() && url.toLocalFile().endsWith(QStringLiteral(".texture"),
                                                           Qt::CaseInsensitive);
}

} // namespace

// Exists only to reach setViewportMargins(), which QAbstractScrollArea keeps
// protected. The grid uses it to park the pixels left over from dividing the
// row into whole columns, half at each end, instead of letting them all collect
// past the last card.
class LauncherGridView : public QListView {
public:
    using QListView::QListView;

    void setLeadingMargin(int margin)
    {
        if (margin == leading)
            return;
        leading = margin;
        setViewportMargins(margin, 0, 0, 0);
    }

private:
    int leading = 0;
};

LauncherWindow::LauncherWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle(QStringLiteral("TextureLab"));
    setMinimumSize(800, 600);
    resize(1280, 820);
    setAcceptDrops(true);

    model = new TextureListModel(this);
    cardDelegate = new TextureCardDelegate(this);
    rowDelegate = new TextureRowDelegate(this);

    const QString currentVersion = libVersionToString(currentLibVersion());
    model->setCurrentLibVersion(currentVersion);
    cardDelegate->setCurrentVersionLabel(currentVersion);
    rowDelegate->setCurrentVersionLabel(currentVersion);

    CatalogService& catalog = CatalogService::instance();
    if (catalog.isReady()) {
        model->setIndex(&catalog.index());
        model->setThumbnailCache(&catalog.thumbnails());
    }

    // The index changes from save, open, and the reconciliation pass; the grid
    // follows rather than polling.
    connect(&catalog, &CatalogService::catalogChanged, this, &LauncherWindow::refresh);

    grid = new LauncherGridView(this);
    grid->setObjectName(QLatin1String(kGridName));
    grid->setModel(model);
    grid->setViewMode(QListView::IconMode);
    grid->setResizeMode(QListView::Adjust);
    grid->setMovement(QListView::Static);
    grid->setUniformItemSizes(true);
    grid->setSelectionMode(QAbstractItemView::ExtendedSelection);
    grid->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    grid->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    grid->setMouseTracking(true);
    grid->setSpacing(6);
    grid->setContextMenuPolicy(Qt::CustomContextMenu);
    grid->setFrameShape(QFrame::NoFrame);

    connect(grid, &QListView::doubleClicked, this, [this]() { openSelected(); });
    connect(grid, &QWidget::customContextMenuRequested, this, &LauncherWindow::showContextMenu);

    // Sits over the grid rather than replacing it, so switching filters can't
    // leave the window structurally empty.
    emptyPanel = new QWidget(grid);
    emptyPanel->setObjectName(QLatin1String(kEmptyPanelName));
    emptyPanel->hide();

    auto* emptyLayout = new QVBoxLayout(emptyPanel);
    emptyLayout->setContentsMargins(24, 24, 24, 24);
    emptyLayout->setSpacing(16);
    emptyLayout->addStretch(1);

    emptyLabel = new QLabel(emptyPanel);
    emptyLabel->setObjectName(QLatin1String(kEmptyLabelName));
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setWordWrap(true);
    emptyLayout->addWidget(emptyLabel);

    // The way out of a first-run window. Same signal as the action bar's
    // button, so the caller can't tell which one the user pressed.
    emptyNewButton = new QPushButton(tr("New Texture"), emptyPanel);
    emptyNewButton->setProperty("variant", "primary"); // the one thing to do here
    emptyNewButton->setCursor(Qt::PointingHandCursor);
    connect(emptyNewButton, &QPushButton::clicked, this, &LauncherWindow::newTextureRequested);
    emptyLayout->addWidget(emptyNewButton, 0, Qt::AlignHCenter);
    emptyLayout->addStretch(1);

    grid->viewport()->installEventFilter(this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(buildTopBar());
    layout->addWidget(grid, 1);
    layout->addWidget(buildActionBar());

    connect(model, &QAbstractItemModel::modelReset, this, &LauncherWindow::updateEmptyState);
    connect(model, &QAbstractItemModel::rowsInserted, this, &LauncherWindow::updateEmptyState);

    // Whether the grid scrolls decides whether it reserves room for a scrollbar,
    // and that follows the row count, which moves without the window resizing.
    connect(model, &QAbstractItemModel::modelReset, this, &LauncherWindow::relayoutGrid);
    connect(model, &QAbstractItemModel::rowsInserted, this, &LauncherWindow::relayoutGrid);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &LauncherWindow::relayoutGrid);

    // Last, so it can drive widgets the two build* methods created.
    restoreViewState();
    updateEmptyState();

    updates = new UpdateChecker(this);
    connect(updates, &UpdateChecker::updateAvailable, this, &LauncherWindow::showUpdateNotice);
}

LauncherWindow::~LauncherWindow() = default;

QWidget* LauncherWindow::buildTopBar()
{
    auto* bar = new QWidget(this);
    bar->setObjectName(QLatin1String(kTopBarName));

    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);

    // All · Recents · Starred — the saved views, as three text tabs rather than
    // a sidebar.
    auto* group = new QButtonGroup(bar);
    group->setExclusive(true);

    auto makeTab = [&](const QString& text, catalog::Filter filter, bool checked) {
        auto* tab = new QToolButton(bar);
        tab->setObjectName(QLatin1String(kFilterTabName));
        tab->setText(text);
        tab->setCheckable(true);
        tab->setChecked(checked);
        tab->setCursor(Qt::PointingHandCursor);
        group->addButton(tab);
        layout->addWidget(tab);
        connect(tab, &QToolButton::clicked, this, [this, filter]() {
            model->setFilter(filter);
            updateEmptyState();
            saveViewState();
        });
        return tab;
    };

    allTab = makeTab(QStringLiteral("All"), catalog::Filter::All, true);
    recentsTab = makeTab(QStringLiteral("Recents"), catalog::Filter::Recents, false);
    starredTab = makeTab(QStringLiteral("Starred"), catalog::Filter::Starred, false);

    layout->addStretch(1);

    search = new QLineEdit(bar);
    search->setPlaceholderText(QStringLiteral("Search…"));
    search->setClearButtonEnabled(true);
    search->setFixedWidth(240);
    connect(search, &QLineEdit::textChanged, this, [this](const QString& text) {
        model->setSearchTerm(text);
        updateEmptyState();
    });
    layout->addWidget(search);

    // Sort dropdown, parked for now. sortBox stays null while this is commented
    // out, and every other use of it is null-guarded, so the launcher just runs
    // on the model's default order (Last Modified, newest first). Uncomment to
    // bring it back — applySort() and the saved "sort" setting are still wired.
    //
    // sortBox = new QComboBox(bar);
    // sortBox->addItem(QStringLiteral("Last Modified"));
    // sortBox->addItem(QStringLiteral("Last Opened"));
    // sortBox->addItem(QStringLiteral("Name"));
    // sortBox->addItem(QStringLiteral("Size"));
    // connect(sortBox, &QComboBox::currentIndexChanged, this, [this](int comboIndex) {
    //     applySort(comboIndex);
    //     saveViewState();
    // });
    // layout->addWidget(sortBox);

    // Update notice: absent entirely until a newer release exists, so the bar
    // stays quiet in the normal case.
    updateButton = new QToolButton(bar);
    updateButton->setObjectName(QLatin1String(kUpdateButtonName));
    updateButton->setCursor(Qt::PointingHandCursor);
    // The icon's stroke is white, which is what the accent-filled button wants;
    // it is not recolored by the theme, so it must not be used on a light fill.
    updateButton->setIcon(QIcon(QStringLiteral(":/icons/download.svg")));
    updateButton->setIconSize(QSize(14, 14));
    updateButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    updateButton->setVisible(false);
    layout->addWidget(updateButton);

    auto* gear = new QToolButton(bar);
    gear->setText(QStringLiteral("⚙"));
    gear->setPopupMode(QToolButton::InstantPopup);
    auto* menu = new QMenu(gear);
    menu->addAction(QStringLiteral("Check for Updates…"), this, [this]() {
        if (!UpdateChecker::isEnabled()) {
            QMessageBox::information(
                this, tr("Check for Updates"),
                tr("Update checks are turned off.\n\nTurn them back on from this menu to let "
                   "TextureLab ask texturelab.io whether a newer build exists."));
            return;
        }
        // A manual check has to say something either way; the automatic one
        // stays silent unless there's news.
        connect(
            updates, &UpdateChecker::checkFinished, this,
            [this](bool found, const QString& error) {
                if (found)
                    return; // the notice in the bar is the answer
                // Both outcomes name the endpoint that was actually used. It is
                // set at compile time and overridable by the environment, so
                // "which server did it ask?" is otherwise unanswerable from
                // inside the app — and that is exactly the question you have
                // when a dev server sees no traffic.
                const QString endpoint =
                    tr("Checked: %1 (%2 channel)")
                        .arg(UpdateChecker::apiBase(), UpdateChecker::channel());

                if (error.isEmpty()) {
                    QMessageBox::information(
                        this, tr("Check for Updates"),
                        tr("TextureLab is up to date.\n\n%1").arg(endpoint));
                }
                else {
                    QMessageBox::warning(this, tr("Check for Updates"),
                                         tr("Could not reach the update server.\n\n%1\n\n%2")
                                             .arg(error, endpoint));
                }
            },
            Qt::SingleShotConnection);

        updates->check(/*force=*/true);
    });

    auto* toggleChecks = menu->addAction(QStringLiteral("Check for Updates Automatically"));
    toggleChecks->setCheckable(true);
    toggleChecks->setChecked(UpdateChecker::isEnabled());
    connect(toggleChecks, &QAction::toggled, this, [](bool on) { UpdateChecker::setEnabled(on); });

    menu->addSeparator();
    menu->addAction(QStringLiteral("Clear Missing Textures"), this, [this]() {
        CatalogService& catalog = CatalogService::instance();
        if (!catalog.isReady())
            return;
        const int removed = catalog.index().removeAllMissing();
        if (removed > 0)
            refresh();
    });
    gear->setMenu(menu);
    layout->addWidget(gear);

    return bar;
}

QWidget* LauncherWindow::buildActionBar()
{
    auto* bar = new QWidget(this);
    bar->setObjectName(QLatin1String(kActionBarName));

    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);

    // ⊞ / ☰ — same model, different delegate. This is the payoff for keeping
    // formatting out of the model.
    auto* viewGroup = new QButtonGroup(bar);
    viewGroup->setExclusive(true);

    auto makeViewToggle = [&](const QString& glyph, const QString& tip, bool checked) {
        auto* button = new QToolButton(bar);
        button->setObjectName(QLatin1String(kFilterTabName));
        button->setText(glyph);
        button->setToolTip(tip);
        button->setCheckable(true);
        button->setChecked(checked);
        button->setCursor(Qt::PointingHandCursor);
        viewGroup->addButton(button);
        layout->addWidget(button);
        return button;
    };

    gridToggle = makeViewToggle(QStringLiteral("⊞"), tr("Grid"), true);
    listToggle = makeViewToggle(QStringLiteral("☰"), tr("List"), false);
    connect(gridToggle, &QToolButton::clicked, this, [this]() { setGridMode(true); });
    connect(listToggle, &QToolButton::clicked, this, [this]() { setGridMode(false); });

    sizeSlider = new QSlider(Qt::Horizontal, bar);
    sizeSlider->setRange(TextureCardDelegate::MinCardWidth, TextureCardDelegate::MaxCardWidth);
    sizeSlider->setValue(targetCardWidth);
    sizeSlider->setFixedWidth(120);
    sizeSlider->setToolTip(tr("Card size — cards stretch to fill the row"));
    connect(sizeSlider, &QSlider::valueChanged, this, [this](int value) {
        // A target, not the drawn width: relayoutGrid() turns it into a column
        // count and hands the delegate whatever divides the viewport evenly.
        targetCardWidth = value;
        relayoutGrid();
        saveViewState();
    });
    layout->addWidget(sizeSlider);

    layout->addStretch(1);

    auto* newButton = new QPushButton(QStringLiteral("New Texture"), bar);
    connect(newButton, &QPushButton::clicked, this, &LauncherWindow::newTextureRequested);
    layout->addWidget(newButton);

    openButton = new QPushButton(QStringLiteral("Open"), bar);
    openButton->setDefault(true);
    openButton->setAutoDefault(true);
    connect(openButton, &QPushButton::clicked, this, [this]() { openSelected(); });
    layout->addWidget(openButton);

    return bar;
}

void LauncherWindow::applySort(int comboIndex)
{
    switch (comboIndex) {
    case 1:
        model->setSort(catalog::SortKey::Opened, false);
        break;
    case 2:
        model->setSort(catalog::SortKey::Name, true);
        break;
    case 3:
        model->setSort(catalog::SortKey::Size, false);
        break;
    default:
        model->setSort(catalog::SortKey::Modified, false);
        break;
    }
}

void LauncherWindow::relayoutGrid()
{
    if (!gridMode || !grid || !cardDelegate || relayouting)
        return;

    // Setting the viewport margins below resizes the viewport, and a viewport
    // resize is what calls this — so the pass has to be allowed to finish
    // before the one it provokes can start.
    const QScopedValueRollback<bool> guard(relayouting, true);

    const int spacing = grid->spacing();

    // grid->width(), not the viewport's: the viewport narrows when the
    // scrollbar appears, and whether the scrollbar appears is one of the things
    // this function decides. Measuring the frame keeps the input to the
    // calculation independent of its own output.
    const int total = grid->width() - grid->frameWidth() * 2;
    if (total <= 0)
        return;

    // Columns from the target width, then the pitch widened to consume the
    // remainder, so the leftover lands in the thumbnails instead of collecting
    // as a ragged right margin.
    //
    // Stated as an explicit grid size rather than left to icon mode's own
    // packing: with a grid size set the view fits exactly usable / pitch cells,
    // which is a rule this can invert. Its default wrapping folds in the item
    // margins and a fencepost, and being one pixel over there costs a whole
    // column silently.
    auto fit = [&](int available) {
        // A grid cell is the card plus one spacing, and the row is indented by
        // half of one before the first cell — the card sits centered in its
        // cell, so the leading half-gutter is not real estate the columns can
        // have.
        const int usable = available - spacing / 2;

        int columns = qMax(1, usable / (targetCardWidth + spacing));
        int width = usable / columns - spacing;

        // A maxed-out slider on a wide window would otherwise stretch past what
        // the delegate is willing to draw, which puts the gap straight back. An
        // extra column costs every card a few pixels and the row nothing.
        while (width > TextureCardDelegate::MaxCardWidth) {
            ++columns;
            width = usable / columns - spacing;
        }

        return qMakePair(columns, qMax(width, TextureCardDelegate::MinCardWidth));
    };

    const int extent = grid->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr,
                                                  grid->verticalScrollBar());

    // Lay the row out against the full width first, then ask whether that many
    // rows overflow. Narrowing only ever means fewer columns and so more rows,
    // so an answer of "it scrolls" can't be undone by the second pass — no
    // oscillation between the two states.
    QPair<int, int> fitted = fit(total);
    const int rows = (model->rowCount() + fitted.first - 1) / fitted.first;
    const int contentHeight =
        spacing / 2 + rows * (cardDelegate->heightForWidth(fitted.second) + spacing);

    // One row-gap of headroom before committing to no scrollbar: guessing wrong
    // in that direction would leave the last row unreachable, where guessing
    // wrong the other way only costs the strip of margin this is here to
    // reclaim.
    const bool scrolls = contentHeight > grid->viewport()->height() - spacing;
    const int layoutWidth = scrolls ? total - extent : total;
    if (scrolls)
        fitted = fit(layoutWidth);

    // Icon mode deducts the scrollbar's width from every row whenever the
    // policy is ScrollBarAsNeeded, whether or not the bar is actually up — that
    // phantom reservation was the dead strip down the right-hand side. Ask for
    // it only when the bar really is coming; when everything fits there is
    // nothing to scroll and so nothing to reserve.
    grid->setVerticalScrollBarPolicy(scrolls ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);


    const int columns = fitted.first;
    int width = fitted.second;

    // Dividing the row into whole columns leaves up to one pixel per column
    // over, and it all collects past the last card — the asymmetry that reads
    // as an odd right-hand margin. Split it instead, by indenting the leading
    // edge. Measured against the frame width rather than the viewport's, so the
    // margin this sets can't feed back into its own input.
    int leftover = layoutWidth - spacing / 2 - columns * (width + spacing);

    // The row can't be centered on less slack than the half-gutter the wrap
    // rule holds back at the far end. When the division came out nearly exact,
    // giving up a pixel of card width buys that back at a pixel per column.
    if (leftover < spacing / 2 && width > TextureCardDelegate::MinCardWidth) {
        --width;
        leftover += columns;
    }

    // Never more than the slack itself: the columns were fitted to a row this
    // wide, and indenting past what's spare would push the last one off it.
    // Clamped before the split, because the pre-layout pass runs against a
    // hundred-pixel window where the one column already overruns and the slack
    // is negative.
    const int slack = qMax(0, leftover);
    grid->setLeadingMargin(qMin((slack + spacing / 2) / 2, slack));

    // The card width alone can't gate this: a pass can run while the scrollbar
    // is still up from a narrower state and lay out against that viewport, then
    // the bar drops and the next pass computes the same width and would decline
    // to re-fit the row it now has room for.
    if (width == cardDelegate->cardWidth() && grid->viewport()->width() == laidOutWidth)
        return;

    laidOutWidth = grid->viewport()->width();
    cardDelegate->setCardWidth(width);

    // setGridSize() relayouts on its own, but only when the value changes — the
    // viewport-width case above arrives with the same grid size and still needs
    // the row re-fitted.
    grid->setGridSize(QSize(width + spacing, cardDelegate->heightForWidth(width) + spacing));
    grid->doItemsLayout();
}

void LauncherWindow::setGridMode(bool useGrid)
{
    gridMode = useGrid;

    if (useGrid) {
        grid->setItemDelegate(cardDelegate);
        grid->setViewMode(QListView::IconMode);
        grid->setSpacing(6);
        grid->setWordWrap(false);
    }
    else {
        grid->setItemDelegate(rowDelegate);
        grid->setViewMode(QListView::ListMode);
        // Rows are separated by their own hairline, so view spacing would only
        // break the continuous surface a table wants.
        grid->setSpacing(0);

        // Rows size themselves; leaving the card pitch in place would stamp
        // every one of them into a square cell.
        grid->setGridSize(QSize());

        // Rows run the full width of the window; the centering margin is a
        // property of the card row, not of the view.
        grid->setLeadingMargin(0);
    }

    // The card width is a function of the viewport, and in list mode nothing
    // has been maintaining it — recompute before the view asks for sizeHints.
    laidOutWidth = -1;
    relayoutGrid();

    // Icon mode caches item positions; swapping the delegate changes every
    // sizeHint, and only a reset makes the view ask again.
    grid->reset();

    if (gridToggle)
        gridToggle->setChecked(useGrid);
    if (listToggle)
        listToggle->setChecked(!useGrid);
    if (sizeSlider)
        sizeSlider->setEnabled(useGrid); // the row height is fixed

    saveViewState();
}

void LauncherWindow::restoreViewState()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("launcher"));

    if (sizeSlider) {
        const int width = settings.value(QStringLiteral("cardWidth"), targetCardWidth).toInt();
        targetCardWidth = qBound(TextureCardDelegate::MinCardWidth, width,
                                 TextureCardDelegate::MaxCardWidth);
        QSignalBlocker block(sizeSlider);
        sizeSlider->setValue(targetCardWidth);
    }

    if (sortBox) {
        const int sortIndex = settings.value(QStringLiteral("sort"), 0).toInt();
        if (sortIndex >= 0 && sortIndex < sortBox->count()) {
            QSignalBlocker block(sortBox);
            sortBox->setCurrentIndex(sortIndex);
            applySort(sortIndex);
        }
    }

    // Filter last: All is the safe default if the stored value is nonsense.
    const int filter = settings.value(QStringLiteral("filter"), 0).toInt();
    if (filter == 1 && recentsTab) {
        recentsTab->setChecked(true);
        model->setFilter(catalog::Filter::Recents);
    }
    else if (filter == 2 && starredTab) {
        starredTab->setChecked(true);
        model->setFilter(catalog::Filter::Starred);
    }

    setGridMode(settings.value(QStringLiteral("gridMode"), true).toBool());

    settings.endGroup();
}

void LauncherWindow::saveViewState() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("launcher"));
    settings.setValue(QStringLiteral("gridMode"), gridMode);
    // The target, not the stretched result — restoring the latter would let a
    // window resized once permanently redefine what the slider means.
    settings.setValue(QStringLiteral("cardWidth"), targetCardWidth);
    if (sortBox)
        settings.setValue(QStringLiteral("sort"), sortBox->currentIndex());

    int filter = 0;
    if (model->filter() == catalog::Filter::Recents)
        filter = 1;
    else if (model->filter() == catalog::Filter::Starred)
        filter = 2;
    settings.setValue(QStringLiteral("filter"), filter);

    settings.endGroup();
}

void LauncherWindow::refresh()
{
    model->refresh();
    updateEmptyState();
}

void LauncherWindow::updateEmptyState()
{
    if (model->totalCount() > 0) {
        emptyPanel->hide();
        return;
    }

    // Three different nothings, and conflating them is how a first-run window
    // ends up looking broken instead of new.
    QString message;
    bool offerNew = false;
    if (!model->searchTerm().isEmpty()) {
        message = tr("No textures match “%1”").arg(model->searchTerm());
    }
    else if (model->filter() == catalog::Filter::Starred) {
        message = tr("No starred textures yet.\nStar one from its right-click menu.");
    }
    else if (model->filter() == catalog::Filter::Recents) {
        message = tr("Nothing opened yet.");
    }
    else {
        message = tr("No textures yet.\n\nTextures you create or open will appear here.");
        offerNew = true;
    }

    emptyLabel->setText(message);
    // Only on the genuine first run — offering "New Texture" as the answer to a
    // search that found nothing would be answering a different question.
    emptyNewButton->setVisible(offerNew);
    layoutEmptyPanel();
    emptyPanel->show();
    emptyPanel->raise();
}

void LauncherWindow::layoutEmptyPanel()
{
    if (emptyPanel)
        emptyPanel->setGeometry(grid->viewport()->geometry());
}

bool LauncherWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == grid->viewport() && event->type() == QEvent::Resize) {
        // An overlay has to follow the viewport by hand; without this, resizing
        // the window while empty leaves the button parked where the grid used
        // to end.
        layoutEmptyPanel();
        relayoutGrid();
    }
    return QWidget::eventFilter(watched, event);
}

void LauncherWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    refresh();
    search->setFocus();

    // Throttled internally, so reopening the launcher from the Home button all
    // day costs one request every few hours.
    if (updates)
        updates->check();
}

void LauncherWindow::showUpdateNotice(const QString& version, const QString& title,
                                      const QString& downloadUrl)
{
    if (!updateButton)
        return;

    // Fixed label rather than the version number: the button's job is to be
    // recognisable at a glance, and the specific version is detail that belongs
    // in the tooltip next to the release title.
    updateButton->setText(tr("New Version Available"));

    QStringList tip;
    tip << tr("Version %1").arg(version);
    if (!title.isEmpty())
        tip << title;
    tip << downloadUrl;
    updateButton->setToolTip(tip.join(QLatin1Char('\n')));
    updateButton->setVisible(true);

    // Opening the browser is the whole action — the launcher never downloads or
    // installs anything on the user's behalf.
    disconnect(updateButton, &QToolButton::clicked, nullptr, nullptr);
    connect(updateButton, &QToolButton::clicked, this,
            [downloadUrl]() { QDesktopServices::openUrl(QUrl(downloadUrl)); });
}

void LauncherWindow::openSelected()
{
    const QModelIndexList selection = grid->selectionModel()->selectedIndexes();

    // With nothing selected, Open falls back to the file dialog. That's also
    // how a texture the launcher has never seen gets in, so it must never be
    // disabled (LAUNCHER_PRD.md §3.1).
    if (selection.isEmpty()) {
        emit openDialogRequested();
        return;
    }

    const catalog::TextureRecord rec = model->recordAt(selection.first());
    if (rec.path.isEmpty())
        return;

    if (!QFileInfo::exists(rec.path)) {
        // Offer to fix it rather than just reporting the problem. Moves aren't
        // detected automatically, so this is how a relocated texture keeps its
        // stars and history.
        const auto answer = QMessageBox::question(
            this, tr("Texture Not Found"),
            tr("This texture is no longer at:\n%1\n\nIf you moved it, you can point the "
               "launcher at its new location.")
                .arg(rec.path),
            QMessageBox::Cancel | QMessageBox::Open, QMessageBox::Open);

        if (answer == QMessageBox::Open)
            locate(rec);
        return;
    }

    emit openPathRequested(rec.path);
}

void LauncherWindow::locate(const catalog::TextureRecord& rec)
{
    CatalogService& catalog = CatalogService::instance();
    if (!catalog.isReady())
        return;

    // Start where it used to live: a moved file is usually a sibling of its old
    // home, or the user at least remembers the neighbourhood.
    const QString startDir = QFileInfo(rec.path).absolutePath();

    const QString chosen = QFileDialog::getOpenFileName(
        this, tr("Locate “%1”").arg(rec.name), startDir,
        tr("Texturelab File (*.texture)"));

    if (chosen.isEmpty())
        return;

    const QFileInfo info(chosen);
    const qint64 survivor = catalog.index().relocate(rec.id, info.absoluteFilePath(), info.size(),
                                                     info.lastModified().toMSecsSinceEpoch());

    if (survivor < 0) {
        QMessageBox::warning(this, tr("Locate Texture"),
                             tr("Could not update the launcher entry:\n%1")
                                 .arg(catalog.index().lastError()));
        return;
    }

    // The file at the new path may be different content entirely, so the old
    // thumbnail can't be trusted. It regenerates on the next open or save.
    catalog.thumbnails().removeTexture(survivor);

    refresh();

    const QModelIndex found = model->indexForId(survivor);
    if (found.isValid()) {
        grid->setCurrentIndex(found);
        grid->scrollTo(found);
    }
}

void LauncherWindow::showContextMenu(const QPoint& pos)
{
    const QModelIndex index = grid->indexAt(pos);
    if (!index.isValid())
        return;

    if (!grid->selectionModel()->isSelected(index))
        grid->setCurrentIndex(index);

    const catalog::TextureRecord rec = model->recordAt(index);

    QMenu menu(this);
    menu.addAction(tr("Open"), this, [this]() { openSelected(); });
    if (rec.isMissing())
        menu.addAction(tr("Locate…"), this, [this, rec]() { locate(rec); });
    menu.addSeparator();
    menu.addAction(rec.starred ? tr("Unstar") : tr("Star"), this,
                   [this]() { toggleStarOnSelection(); });
    menu.addAction(tr("Show in File Manager"), this, [rec]() {
        // Opens the containing directory; selecting the file itself needs
        // per-platform shell calls that aren't worth it here.
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(rec.path).absolutePath()));
    });
    menu.addSeparator();
    menu.addAction(tr("Remove from Launcher"), this,
                   [this]() { removeSelectionFromLauncher(); });

    menu.exec(grid->viewport()->mapToGlobal(pos));
}

void LauncherWindow::toggleStarOnSelection()
{
    CatalogService& catalog = CatalogService::instance();
    if (!catalog.isReady())
        return;

    const QModelIndexList selection = grid->selectionModel()->selectedIndexes();
    if (selection.isEmpty())
        return;

    // One toggle for the whole selection, driven by the first item, so a
    // multi-select doesn't half-star and half-unstar.
    const bool starred = model->recordAt(selection.first()).starred;
    for (const QModelIndex& index : selection) {
        const catalog::TextureRecord rec = model->recordAt(index);
        if (rec.isValid())
            catalog.index().setStarred(rec.id, !starred);
    }

    refresh();
}

void LauncherWindow::removeSelectionFromLauncher()
{
    CatalogService& catalog = CatalogService::instance();
    if (!catalog.isReady())
        return;

    const QModelIndexList selection = grid->selectionModel()->selectedIndexes();
    if (selection.isEmpty())
        return;

    const auto answer = QMessageBox::question(
        this, tr("Remove from Launcher"),
        tr("Remove %n texture(s) from the launcher?\n\nThe files stay on disk — this only "
           "forgets them here.",
           nullptr, int(selection.size())),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    for (const QModelIndex& index : selection) {
        const catalog::TextureRecord rec = model->recordAt(index);
        if (rec.isValid())
            catalog.forget(rec.id);
    }

    refresh();
}

void LauncherWindow::setOpenPath(const QString& path)
{
    model->setOpenPath(path);
}

void LauncherWindow::setHasDocument(bool value)
{
    hasDocument = value;
}

void LauncherWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Find)) {
        search->setFocus();
        search->selectAll();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        // Clear the search first if there is one; only then close. And never
        // close when there's no document behind us — that would leave the user
        // staring at nothing.
        if (!search->text().isEmpty()) {
            search->clear();
            event->accept();
            return;
        }
        if (hasDocument)
            emit closeRequested();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        openSelected();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void LauncherWindow::closeEvent(QCloseEvent* event)
{
    if (!hasDocument) {
        // The window manager's close button on first launch means "quit", not
        // "show me the empty editor behind this".
        event->accept();
        return;
    }

    event->ignore();
    emit closeRequested();
}

void LauncherWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    for (const QUrl& url : event->mimeData()->urls()) {
        if (isTextureFile(url)) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void LauncherWindow::dropEvent(QDropEvent* event)
{
    // Dropping a .texture adds it and opens it — the explicit recovery path if
    // index.db is ever lost (LAUNCHER_PRD.md §1.1).
    for (const QUrl& url : event->mimeData()->urls()) {
        if (isTextureFile(url)) {
            event->acceptProposedAction();
            emit openPathRequested(url.toLocalFile());
            return;
        }
    }
    event->ignore();
}
