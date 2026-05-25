#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "DockManager.h"
#include <QMainWindow>
#include <QMenu>
#include <QSettings>
#include <QSharedPointer>
#include <QString>

class GraphWidget;
class LibraryWidget;
class PropertiesWidget;
class View2DWidget;
class View3DWidget;
class TextureRenderer;
class ExportDialog;

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

protected:
    void setupToolbar();
    void setupMenus();
    void setupDocks();

    // menu callbacks
    void openProject();
    void newProject();
    void saveProject();
    void saveProjectAs();
    void showExportDialog();
    void directExport();
    void handleExport(const QString& destination, const QString& pattern);

    void passTextureChannelsToViewer3D();

    void setProject(TextureProjectPtr project);

    void addToRecentFiles(const QString& filePath);
    void updateRecentFilesMenu();

    ads::CDockAreaWidget* addDock(const QString& title,
                                  ads::DockWidgetArea area, QWidget* widget,
                                  ads::CDockAreaWidget* areaWidget);

private:
    static constexpr int MaxRecentFiles = 10;

    ads::CDockManager* dockManager;
    QMenu* recentFilesMenu;
    QToolBar* toolBar;
    QWidget* editor;

    GraphWidget* graphWidget;
    LibraryWidget* libraryWidget;
    PropertiesWidget* propWidget;
    View2DWidget* view2DWidget;
    View3DWidget* view3DWidget;
    ExportDialog* exportDialog;

    TextureRenderer* renderer;

    QProgressBar* progressBar;
    QLabel* statusLabel;

    TextureProjectPtr project;
};
#endif // MAINWINDOW_H
