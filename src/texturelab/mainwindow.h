#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include "DockManager.h"

class GraphWidget;
class LibraryWidget;
class PropertiesWidget;
class View2DWidget;
class View3DWidget;

class QToolBar;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void setupToolbar();
    void setupMenus();
    void setupDocks();

    ads::CDockAreaWidget *addDock(const QString &title, ads::DockWidgetArea area, QWidget *widget, ads::CDockAreaWidget *areaWidget);

private:
    ads::CDockManager *dockManager;
    QToolBar *toolBar;
    QWidget *editor;

    GraphWidget *graphWidget;
    LibraryWidget *libraryWidget;
    PropertiesWidget *propWidget;
    View2DWidget *view2DWidget;
    View3DWidget *view3DWidget;
};
#endif // MAINWINDOW_H
