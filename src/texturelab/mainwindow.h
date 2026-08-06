#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "DockManager.h"
#include "models.h"
#include <QCloseEvent>
#include <QMap>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMainWindow>
#include <QMenu>
#include <QSettings>
#include <QSharedPointer>
#include <QString>
#include <QUndoStack>

class GraphWidget;
class LibraryWidget;
class PropertiesWidget;
class View2DWidget;
class View3DWidget;
class TextureRenderer;
class ExportDialog;
class LauncherWindow;

class TextureProject;
typedef QSharedPointer<TextureProject> TextureProjectPtr;

class QToolBar;
class QProgressBar;
class QLabel;
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // Shows the launcher over the editor. Called at startup (before this window
    // is ever shown) and from the Home button in the status bar.
    void showLauncher();

protected:
    void setupToolbar();
    void setupMenus();
    void setupDocks();
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    // menu callbacks
    void openProject();

    // Shared by the Open dialog, recent-files menu, and drag-and-drop:
    // prompts to save if dirty, reads the file, offers the
    // version-upgrade dialog if needed, then loads it.
    void openProjectFromPath(const QString& filePath);
    void newProject();
    void saveProject();
    void saveProjectAs();
    void showExportDialog();
    void directExport();
    void handleExport(const QString& destination, const QString& pattern);

    // Grabs the 3D viewport and stores it as this texture's launcher thumbnail.
    // No-op when the viewport has never initialized its GL context, or when the
    // texture isn't in the index.
    void captureLauncherThumbnail(int source);

    void passTextureChannelsToViewer3D();
    void syncChannelLabelsToScene();

    void setProject(TextureProjectPtr project);

    // Upgrades the currently open project's library in place, via the
    // Library dock panel's "Upgrade" button.
    void upgradeCurrentProjectLibrary();

    void addToRecentFiles(const QString& filePath);
    void updateRecentFilesMenu();

    void onCleanChanged(bool clean);

    // Returns false if the user cancelled a "save changes?" dialog.
    bool promptSaveIfDirty();

    ads::CDockAreaWidget* addDock(const QString& title,
                                  ads::DockWidgetArea area, QWidget* widget,
                                  ads::CDockAreaWidget* areaWidget);

private:
    static constexpr int MaxRecentFiles = 10;

    QUndoStack* undoStack;

    ads::CDockManager* dockManager;
    QString adsDefaultStyleSheet; // ADS's own default sheet, captured before we theme it
    QMenu* recentFilesMenu;
    QToolBar* toolBar;
    QWidget* editor;

    GraphWidget* graphWidget;
    LibraryWidget* libraryWidget;
    PropertiesWidget* propWidget;
    View2DWidget* view2DWidget;
    View3DWidget* view3DWidget;
    ExportDialog* exportDialog;

    // Created lazily on first show; owned by this window so it survives being
    // hidden and reopened from the Home button.
    LauncherWindow* launcher = nullptr;

    // Set when a document is opened, cleared once the graph has finished
    // rendering and the thumbnail has been captured. Opening is asynchronous —
    // the viewport shows nothing useful until the last node is clean.
    bool thumbnailCapturePending = false;

    TextureRenderer* renderer;

    QProgressBar* progressBar;
    QLabel* statusLabel;

    TextureProjectPtr project;

    // channel→node mapping last pushed to the 3D viewer and the graph's node
    // labels, so undo-stack moves that didn't touch it can skip the re-sync
    QMap<TextureChannel, QString> syncedChannels;
};
#endif // MAINWINDOW_H
