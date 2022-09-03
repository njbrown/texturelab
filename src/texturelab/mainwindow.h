#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include "DockManager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void setupMenus();
    void setupDocks();

    ads::CDockAreaWidget *addDock(const QString &title, ads::DockWidgetArea area, QWidget *widget, ads::CDockAreaWidget *areaWidget);

private:
    ads::CDockManager *dockManager;
};
#endif // MAINWINDOW_H
