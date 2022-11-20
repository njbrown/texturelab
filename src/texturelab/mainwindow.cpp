#include "mainwindow.h"

#include <QFileDialog>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>

#include "DockAreaWidget.h"
#include "DockSplitter.h"

#include "widgets/graphwidget.h"
#include "widgets/librarywidget.h"
#include "widgets/properties/propertieswidget.h"
#include "widgets/view2dwidget.h"

#include "models.h"
#include "project.h"

#include "graphics/texturerenderer.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    resize(1280, 720);

    this->setupMenus();
    this->setupToolbar();

    this->renderer = nullptr;

    this->dockManager = new ads::CDockManager(this);

    this->setupDocks();

    // setup callbacks for the widgets that are created once
    connect(this->graphWidget, &GraphWidget::nodeSelectionChanged,
            [this](const TextureNodePtr& node) {
                if (!!node) {
                    this->propWidget->setSelectedNode(node);
                    // this->view2DWidget->setSelectedNode(node);
                }
                else {
                    this->propWidget->clearSelection();
                    // this->view2DWidget->clearSelection();
                }
            });

    connect(this->graphWidget, &GraphWidget::nodeDoubleClicked,
            [this](const TextureNodePtr& node) {
                if (!!node) {
                    this->view2DWidget->setSelectedNode(node);
                }
                else {
                    this->view2DWidget->clearSelection();
                }
            });

    connect(this->propWidget, &PropertiesWidget::propertyUpdated,
            [this](const QString& name, const QVariant& value) {
                if (this->renderer && !!this->project) {
                    this->renderer->update();
                }

                this->view2DWidget->reRenderNode();
            });

    // set default empty project
    auto project = TextureProject::createEmpty();
    this->setProject(project);

    // test texture rendering
    //     auto renderer = new TextureRenderer();
    //     renderer->testRendering();
}

void MainWindow::setProject(TextureProjectPtr project)
{
    this->project = project;
    this->graphWidget->setTextureProject(project);
    this->libraryWidget->setLibrary(project->library);

    this->propWidget->clearSelection();
    this->propWidget->setProject(project);

    renderer = new TextureRenderer();
    renderer->setProject(project);
    this->graphWidget->setTextureRenderer(renderer);
    this->view2DWidget->setTextureRenderer(renderer);

    renderer->update();
}

void MainWindow::setupMenus()
{
    auto fileMenu = this->menuBar()->addMenu("File");
    fileMenu->addAction("Open Project", [=]() { this->openProject(); });
    fileMenu->addAction("New Project", [=]() { this->newProject(); });
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

    QWidget* spacer = new QWidget();
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
void setWidgetRatiosInArea(ads::CDockAreaWidget* area,
                           const QList<float>& ratios)
{
    auto splitter = ads::internal::findParent<ads::CDockSplitter*>(area);
    if (splitter) {
        int width = splitter->width();

        QList<int> finalRatios;
        for (auto ratio : ratios) {
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
    this->graphWidget = new GraphWidget();
    this->view2DWidget = new View2DWidget();
    auto graphArea =
        addDock("Graph", ads::CenterDockWidgetArea, graphWidget, nullptr);
    auto leftArea = addDock("2D View", ads::LeftDockWidgetArea,
                            this->view2DWidget, graphArea);

    this->propWidget = new PropertiesWidget();
    auto rightArea = addDock("Properties", ads::RightDockWidgetArea,
                             this->propWidget, graphArea);

    this->libraryWidget = new LibraryWidget();
    setWidgetRatiosInArea(graphArea, {1.0f / 5, 3.0f / 5, 1.0f / 5});

    addDock("3D View", ads::BottomDockWidgetArea, new QWidget(this), leftArea);
    addDock("Library", ads::BottomDockWidgetArea, this->libraryWidget,
            rightArea);
    setWidgetRatiosInArea(leftArea, {0.5f, 0.5f});
    setWidgetRatiosInArea(rightArea, {0.5f, 0.5f});
}

ads::CDockAreaWidget* MainWindow::addDock(const QString& title,
                                          ads::DockWidgetArea area,
                                          QWidget* widget,
                                          ads::CDockAreaWidget* areaWidget)
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget(title);
    if (widget != nullptr)
        dockWidget->setWidget(widget);

    auto newAreaWidget =
        dockManager->addDockWidget(area, dockWidget, areaWidget);

    return newAreaWidget;
}

void MainWindow::openProject()
{
    auto filePath = QFileDialog::getOpenFileName(this, "Open Texture File", "",
                                                 "Texturelab File (*.texture)");

    if (filePath.isNull() || filePath.isEmpty()) {
        return;
    }

    auto project = Project::loadTexture(filePath);

    setProject(project);
}

void MainWindow::newProject() { setProject(TextureProject::createEmpty()); }

MainWindow::~MainWindow() {}