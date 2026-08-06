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

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    QWidget* buildTopBar();
    QWidget* buildActionBar();
    void applySort(int comboIndex);
    void setGridMode(bool grid);
    void restoreViewState();
    void saveViewState() const;
    void updateEmptyState();
    void openSelected();

    // Asks the user where a missing texture went and re-points its index row,
    // keeping stars, tags, and recency.
    void locate(const catalog::TextureRecord& rec);

    void showContextMenu(const QPoint& pos);
    void toggleStarOnSelection();
    void removeSelectionFromLauncher();

    TextureListModel* model = nullptr;
    TextureCardDelegate* cardDelegate = nullptr;
    TextureRowDelegate* rowDelegate = nullptr;

    QListView* grid = nullptr;
    QLineEdit* search = nullptr;
    QComboBox* sortBox = nullptr;
    QSlider* sizeSlider = nullptr;
    QLabel* emptyLabel = nullptr;
    QPushButton* openButton = nullptr;
    QToolButton* gridToggle = nullptr;
    QToolButton* listToggle = nullptr;
    QToolButton* allTab = nullptr;
    QToolButton* recentsTab = nullptr;
    QToolButton* starredTab = nullptr;

    bool hasDocument = false;
    bool gridMode = true;
};
