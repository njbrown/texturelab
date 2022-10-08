#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "DockManager.h"
#include <QMainWindow>
#include <QSharedPointer>
#include <QString>

class GraphWidget;
class LibraryWidget;
class PropertiesWidget;
class View2DWidget;
class View3DWidget;

class TextureProject;
typedef QSharedPointer<TextureProject> TextureProjectPtr;

class QToolBar;
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

    void setProject(TextureProjectPtr project);

    ads::CDockAreaWidget* addDock(const QString& title,
                                  ads::DockWidgetArea area, QWidget* widget,
                                  ads::CDockAreaWidget* areaWidget);

private:
    ads::CDockManager* dockManager;
    QToolBar* toolBar;
    QWidget* editor;

    GraphWidget* graphWidget;
    LibraryWidget* libraryWidget;
    PropertiesWidget* propWidget;
    View2DWidget* view2DWidget;
    View3DWidget* view3DWidget;

    TextureProjectPtr project;
};
#endif // MAINWINDOW_H
