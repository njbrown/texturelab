#pragma once

#include "texturerecord.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QListView;
class QPushButton;
class QSlider;
class QToolButton;

class TextureCardDelegate;
class TextureListModel;
class TextureRowDelegate;
class UpdateChecker;

// The launcher: a flat grid of every texture the app has touched.
//
// Knows nothing about MainWindow. It emits what the user asked for and lets the
// caller decide how to honor it — which is what keeps "Open" able to run through
// promptSaveIfDirty() without this window having to know that dirty documents
// are a concept.
class LauncherWindow : public QWidget {
    Q_OBJECT

public:
    explicit LauncherWindow(QWidget* parent = nullptr);
    ~LauncherWindow() override;

    // Marks the document currently loaded in the editor, for the card's "open"
    // dot. Empty when nothing is loaded.
    void setOpenPath(const QString& path);

    // True once a document exists behind the launcher, which is what makes Esc
    // and the close button meaningful — on first launch there is nowhere to go.
    void setHasDocument(bool hasDocument);

signals:
    void newTextureRequested();
    void openPathRequested(const QString& path);
    void openDialogRequested();
    void closeRequested();

public slots:
    void refresh();

    // Reveals the "Update to X" affordance in the top bar. Nothing is
    // downloaded; clicking it opens the release page in the browser.
    void showUpdateNotice(const QString& version, const QString& title,
                          const QString& downloadUrl);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QWidget* buildTopBar();
    QWidget* buildActionBar();
    void applySort(int comboIndex);
    void setGridMode(bool grid);

    // Re-fits the cards to the viewport: columns come from the slider's target
    // width, then every cell stretches to consume the remainder. IconMode's own
    // layout keeps a fixed pitch and dumps the leftover as a ragged right
    // margin, which is the gap this exists to close.
    void relayoutGrid();
    void restoreViewState();
    void saveViewState() const;
    void updateEmptyState();

    // The empty-state overlay isn't a layout child, so it has to be re-fitted
    // to the viewport by hand whenever the grid changes size.
    void layoutEmptyPanel();
    void openSelected();

    // Asks the user where a missing texture went and re-points its index row,
    // keeping stars, tags, and recency.
    void locate(const catalog::TextureRecord& rec);

    void showContextMenu(const QPoint& pos);
    void toggleStarOnSelection();
    void removeSelectionFromLauncher();

    UpdateChecker* updates = nullptr;
    TextureListModel* model = nullptr;
    TextureCardDelegate* cardDelegate = nullptr;
    TextureRowDelegate* rowDelegate = nullptr;

    QListView* grid = nullptr;
    QLineEdit* search = nullptr;
    QComboBox* sortBox = nullptr;
    QSlider* sizeSlider = nullptr;
    QWidget* emptyPanel = nullptr;
    QLabel* emptyLabel = nullptr;
    QPushButton* emptyNewButton = nullptr;
    QPushButton* openButton = nullptr;
    QToolButton* updateButton = nullptr;
    QToolButton* gridToggle = nullptr;
    QToolButton* listToggle = nullptr;
    QToolButton* allTab = nullptr;
    QToolButton* recentsTab = nullptr;
    QToolButton* starredTab = nullptr;

    bool hasDocument = false;
    bool gridMode = true;

    // The viewport width the cards were last laid out against. The card width
    // alone can't gate a relayout: the first pass can run while the scrollbar
    // is still up from a narrower state, fit one column fewer than it computed,
    // and then match on the next pass and decline to fix itself.
    int laidOutWidth = -1;

    // What the slider asks for. The width the cards are actually drawn at is
    // this one rounded to fill the row, and lives on the delegate.
    int targetCardWidth = 160;
};
