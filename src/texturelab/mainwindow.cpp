#include "mainwindow.h"

#include <QList>
#include <QToolBar>
#include <QLayout>
#include <QMenuBar>
#include <QMenu>

#include "DockSplitter.h"
#include "DockAreaWidget.h"

#include "widgets/graphwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1280, 720);

    this->setupMenus();
    this->setupToolbar();

    this->dockManager = new ads::CDockManager(this);

    this->setupDocks();
}

void MainWindow::setupMenus()
{
    auto fileMenu = this->menuBar()->addMenu("File");
    fileMenu->addAction("Open Project", []() {});
    fileMenu->addAction("New Project", []() {});
    fileMenu->addSeparator();
    fileMenu->addAction("Save", []() {});
    fileMenu->addAction("Save As...", []() {});
    fileMenu->addSeparator();
    fileMenu->addAction("Edit", []() {});

    auto editMenu = this->menuBar()->addMenu("Edit");
    editMenu->addAction("Undo", []() {});
    editMenu->addAction("Redo", []() {});
    editMenu->addAction("Cut", []() {});
    editMenu->addAction("Copy", []() {});
    editMenu->addAction("Paste", []() {});

    auto examplesMenu = this->menuBar()->addMenu("Examples");
    auto optionsMenu = this->menuBar()->addMenu("Help");
    optionsMenu->addAction("Documentation", []() {});
    optionsMenu->addAction("About", []() {});
}

void MainWindow::setupToolbar()
{
    // https://www.setnode.com/blog/right-aligning-a-button-in-a-qtoolbar/
    toolBar = this->addToolBar("main toolbar");

    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // undo redo
    toolBar->addAction("Undo");
    toolBar->addAction("Redo");

    // spacer
    toolBar->addWidget(spacer);

    // export
    toolBar->addAction("Export");

    // behavior
    toolBar->setMovable(false);
}

/*
    https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/issues/411
    https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/issues/340

    pass ratios as fractions, the function handles the actual width calculation
    setWidgetRatiosInArea(myArea, { 1.0f/5.0f, 1.0f/5.0f, 1.0f/5.0f })
*/
void setWidgetRatiosInArea(ads::CDockAreaWidget *area, const QList<float> &ratios)
{
    auto splitter = ads::internal::findParent<ads::CDockSplitter *>(area);
    if (splitter)
    {
        int width = splitter->width();

        QList<int> finalRatios;
        for (auto ratio : ratios)
        {
            finalRatios.append(ratio * width);
        }
        splitter->setSizes(finalRatios);
    }
}

void MainWindow::setupDocks()
{
    setDockNestingEnabled(true);

    // https://forum.qt.io/topic/3055/mainwindow-layout-problem-with-qdockwidget/17

    // graph goes in the center
    auto graphArea = addDock("Graph", ads::CenterDockWidgetArea, new GraphWidget(), nullptr);
    auto leftArea = addDock("2D View", ads::LeftDockWidgetArea, new QWidget(this), graphArea);
    auto rightArea = addDock("Properties", ads::RightDockWidgetArea, new QWidget(this), graphArea);

    setWidgetRatiosInArea(graphArea, {1.0f / 5, 3.0f / 5, 1.0f / 5});

    addDock("3D View", ads::BottomDockWidgetArea, new QWidget(this), leftArea);
    addDock("Library", ads::BottomDockWidgetArea, new QWidget(this), rightArea);
    setWidgetRatiosInArea(leftArea, {0.5f, 0.5f});
    setWidgetRatiosInArea(rightArea, {0.5f, 0.5f});
}

ads::CDockAreaWidget *MainWindow::addDock(const QString &title, ads::DockWidgetArea area, QWidget *widget, ads::CDockAreaWidget *areaWidget)
{
    ads::CDockWidget *dockWidget = new ads::CDockWidget(title);
    if (widget != nullptr)
        dockWidget->setWidget(widget);

    auto newAreaWidget = dockManager->addDockWidget(area, dockWidget, areaWidget);

    return newAreaWidget;
}

MainWindow::~MainWindow()
{
}