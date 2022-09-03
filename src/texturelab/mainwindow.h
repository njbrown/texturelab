#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include "DockManager.h"

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
    QWidget* editor;
};
#endif // MAINWINDOW_H
