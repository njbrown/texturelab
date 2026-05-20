#include "mainwindow.h"

#include <vector>

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPushButton>
#include <QToolBar>
#include <QToolButton>

#include "DockAreaWidget.h"
#include "DockSplitter.h"

#include "exporter.h"
#include "widgets/exportdialog.h"
#include "widgets/graphwidget.h"
#include "widgets/librarywidget.h"
#include "widgets/properties/propertieswidget.h"
#include "widgets/view2dwidget.h"
#include "widgets/view3dwidget.h"

#include "viewer3d.h"

#include "models.h"
#include "project.h"
#include "props.h"

#include "graphics/texturerenderer.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    resize(1280, 720);

    this->setupMenus();
    this->setupToolbar();

    this->renderer = nullptr;
    this->exportDialog = nullptr;

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
                this->view3DWidget->reRender();
            });

    connect(this->propWidget, &PropertiesWidget::textureChannelUpdated,
            [this](const TextureChannel& name, const TextureNodePtr& node) {
                if (this->renderer && !!this->project) {
                    this->renderer->update();
                }

                if (!!this->project) {
                    if (name == TextureChannel::None) {
                        // Find and remove any channel that points to this node
                        auto it = this->project->textureChannels.begin();
                        while (it != this->project->textureChannels.end()) {
                            if (it.value() == node->id) {
                                it = this->project->textureChannels.erase(it);
                            }
                            else {
                                ++it;
                            }
                        }
                    }
                    else {

                        this->project->textureChannels[name] = node->id;
                    }

                    // assign channel to viewer
                    // do this crudely by just reassigning all node textures
                    this->passTextureChannelsToViewer3D();
                }

                // this->view3DWidget->update();
                this->view3DWidget->reRender();
            });

    // set default empty project
    auto project = TextureProject::createEmpty();
    this->setProject(project);

    // Print GPU information
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (context) {
        QOpenGLFunctions* f = context->functions();
        const GLubyte* vendor = f->glGetString(GL_VENDOR);
        const GLubyte* renderer = f->glGetString(GL_RENDERER);
        const GLubyte* version = f->glGetString(GL_VERSION);

        qDebug() << "=== GPU Information ===";
        qDebug() << "GPU Vendor:" << reinterpret_cast<const char*>(vendor);
        qDebug() << "GPU Renderer:" << reinterpret_cast<const char*>(renderer);
        qDebug() << "OpenGL Version:" << reinterpret_cast<const char*>(version);
        qDebug() << "======================";
    }
    else {
        qDebug() << "Warning: No OpenGL context available yet";
    }

    // test texture rendering
    //     auto renderer = new TextureRenderer();
    //     renderer->testRendering();
}

void MainWindow::passTextureChannelsToViewer3D()
{
    auto viewer = this->view3DWidget->viewer;
    viewer->clearTextures();

    for (auto channel : this->project->textureChannels.keys()) {
        auto nodeId = this->project->textureChannels[channel];

        auto node = this->project->getNodeById(nodeId);
        if (!node)
            continue;

        switch (channel) {
        case TextureChannel::Albedo:
            viewer->setAlbedoTexture(node->textureId());
            break;
        case TextureChannel::Normal:
            viewer->setNormalTexture(node->textureId());
            break;
        case TextureChannel::Metalness:
            viewer->setMetalnessTexture(node->textureId());
            break;
        case TextureChannel::Roughness:
            viewer->setRoughnessTexture(node->textureId());
            break;
        case TextureChannel::Height:
            viewer->setHeightTexture(node->textureId());
            break;
        case TextureChannel::AO:
            viewer->setAoTexture(node->textureId());
            break;
        default:
            break;
        }
    }
}

void MainWindow::setProject(TextureProjectPtr project)
{
    // Clean up old renderer before creating new one
    if (this->renderer) {
        // Clear references to old renderer in widgets
        this->graphWidget->setTextureRenderer(nullptr);
        this->view2DWidget->setTextureRenderer(nullptr);

        delete this->renderer;
        this->renderer = nullptr;
    }

    this->project = project;
    this->graphWidget->setTextureProject(project);
    this->libraryWidget->setLibrary(project->library);

    this->propWidget->clearSelection();
    this->propWidget->setProject(project);

    renderer = new TextureRenderer();
    renderer->setProject(project);
    this->graphWidget->setTextureRenderer(renderer);
    this->view2DWidget->setTextureRenderer(renderer);

    // Update view3D textures when a node's texture is updated
    connect(renderer, &TextureRenderer::thumbnailGenerated,
            [this](const QString& nodeId, GLuint texId, const QPixmap&) {
                if (!this->project)
                    return;

                auto viewer = this->view3DWidget->viewer;
                for (auto channel : this->project->textureChannels.keys()) {
                    if (this->project->textureChannels[channel] == nodeId) {
                        switch (channel) {
                        case TextureChannel::Albedo:
                            viewer->setAlbedoTexture(texId);
                            break;
                        case TextureChannel::Normal:
                            viewer->setNormalTexture(texId);
                            break;
                        case TextureChannel::Metalness:
                            viewer->setMetalnessTexture(texId);
                            break;
                        case TextureChannel::Roughness:
                            viewer->setRoughnessTexture(texId);
                            break;
                        case TextureChannel::Height:
                            viewer->setHeightTexture(texId);
                            break;
                        case TextureChannel::AO:
                            viewer->setAoTexture(texId);
                            break;
                        default:
                            break;
                        }
                        this->view3DWidget->reRender();
                        break;
                    }
                }
            });

    renderer->update();

    // Update window title with project name
    setWindowTitle(project->name + " - TextureLab");
}

void MainWindow::setupMenus()
{
    auto fileMenu = this->menuBar()->addMenu("File");
    fileMenu->addAction("Open Project", [=]() { this->openProject(); });
    fileMenu->addAction("New Project", [=]() { this->newProject(); });
    fileMenu->addSeparator();
    fileMenu->addAction("Save", [=]() { this->saveProject(); });
    fileMenu->addAction("Save As...", [=]() { this->saveProjectAs(); });
    fileMenu->addSeparator();
    fileMenu->addAction("Edit", []() {});

    auto editMenu = this->menuBar()->addMenu("Edit");
    editMenu->addAction("Undo", []() {});
    editMenu->addAction("Redo", []() {});
    editMenu->addAction("Cut", []() {});
    editMenu->addAction("Copy", []() {});
    editMenu->addAction("Paste", []() {});

    auto examplesMenu = this->menuBar()->addMenu("Examples");

    // List of example texture files
    QStringList examples = {"Copper.texture",
                            "FoilGasket.texture",
                            "GoldLinedMarbleTiles.texture",
                            "Grass.texture",
                            "GrassyRock.texture",
                            "Grenade.texture",
                            "Sand.texture",
                            "Screws.texture",
                            "WoodenPlanks.texture",
                            "YellowTiles.texture"};

    for (const QString& example : examples) {
        // Remove .texture extension for display name
        QString displayName = example;
        displayName.replace(".texture", "");

        examplesMenu->addAction(displayName, [this, example]() {
            QString examplePath = ":examples/" + example;

            // Load the example project
            auto project = Project::loadTexture(examplePath);

            // Set project name from filename
            QString projectName = example;
            projectName.replace(".texture", "");
            project->name = projectName;

            setProject(project);
        });
    }

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

    // Export button with dropdown menu
    auto exportBtn = new QToolButton(this);
    exportBtn->setText("Export");
    exportBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    auto directExportAction = new QAction("Export", this);
    connect(directExportAction, &QAction::triggered, this,
            &MainWindow::directExport);

    auto settingsAction = new QAction("Export Settings...", this);
    connect(settingsAction, &QAction::triggered, this,
            &MainWindow::showExportDialog);

    auto exportMenu = new QMenu(this);
    exportMenu->addAction(settingsAction);

    exportBtn->setDefaultAction(directExportAction);
    exportBtn->setMenu(exportMenu);
    exportBtn->setPopupMode(QToolButton::MenuButtonPopup);

    toolBar->addWidget(exportBtn);

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
    this->view3DWidget = new View3DWidget();

    auto graphArea =
        addDock("Graph", ads::CenterDockWidgetArea, graphWidget, nullptr);
    auto leftArea = addDock("2D View", ads::LeftDockWidgetArea,
                            this->view2DWidget, graphArea);

    this->propWidget = new PropertiesWidget();
    auto rightArea = addDock("Properties", ads::RightDockWidgetArea,
                             this->propWidget, graphArea);

    this->libraryWidget = new LibraryWidget();
    setWidgetRatiosInArea(graphArea, {1.0f / 5, 3.0f / 5, 1.0f / 5});

    addDock("3D View", ads::BottomDockWidgetArea, this->view3DWidget, leftArea);
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
    // ads::CDockWidget* dockWidget = new ads::CDockWidget(title);
    ads::CDockWidget* dockWidget = dockManager->createDockWidget(title);
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

    // Extract filename without extension
    QFileInfo fileInfo(filePath);
    project->name = fileInfo.baseName();

    setProject(project);
}

void MainWindow::newProject() { setProject(TextureProject::createEmpty()); }

void MainWindow::saveProject()
{
    if (project->filePath.isNull() || project->filePath.isEmpty()) {
        QString filePath = QFileDialog::getSaveFileName(
            this, "Save Texture...", QString(), "Texturelab File (*.texture)");

        if (filePath.isNull() || filePath.isEmpty()) {
            return;
        }

        if (!filePath.endsWith(".texture", Qt::CaseInsensitive))
            filePath += ".texture";

        project->filePath = filePath;
    }

    QFile file(project->filePath);
    file.open(QIODevice::WriteOnly);
    file.write(Project::saveTexture(project));
    file.close();
}

void MainWindow::saveProjectAs()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, "Save Texture As...", QString(), "Texturelab File (*.texture)");

    if (filePath.isNull() || filePath.isEmpty()) {
        return;
    }

    if (!filePath.endsWith(".texture", Qt::CaseInsensitive))
        filePath += ".texture";

    project->filePath = filePath;

    QFile file(project->filePath);
    file.open(QIODevice::WriteOnly);
    file.write(Project::saveTexture(project));
    file.close();
}

void MainWindow::showExportDialog()
{
    if (!this->exportDialog) {
        this->exportDialog = new ExportDialog(this);
    }

    if (this->project) {
        this->exportDialog->setProject(this->project);
    }

    this->exportDialog->show();
    this->exportDialog->raise();
    this->exportDialog->activateWindow();
}

void MainWindow::directExport()
{
    // Check if we have a valid project
    if (!this->project) {
        showExportDialog();
        return;
    }

    // If no export destination is set, prompt for one
    if (this->project->exportDestination.isEmpty()) {
        QString dir = QFileDialog::getExistingDirectory(
            this, "Select Export Destination", "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        if (dir.isEmpty()) {
            // User cancelled
            return;
        }

        // Save destination to project
        this->project->exportDestination = dir;

        // Also update dialog if it exists
        if (this->exportDialog) {
            this->exportDialog->setProject(this->project);
        }
    }

    // Perform export with project settings
    handleExport(this->project->exportDestination,
                 this->project->exportFilePattern);
}

void MainWindow::handleExport(const QString& destination,
                              const QString& pattern)
{
    if (!this->project || !this->renderer) {
        QMessageBox::warning(this, "Export Error",
                             "No project loaded or renderer not initialized.");
        return;
    }

    // Find all output nodes
    QVector<TextureNodePtr> outputNodes;
    for (auto& node : this->project->nodes) {
        // Check if this is an output node by checking the library name
        if (node->title == "Output") {
            outputNodes.append(node);
        }
    }

    if (outputNodes.isEmpty()) {
        QMessageBox::information(this, "Export",
                                 "No output nodes found in the project.");
        return;
    }

    // Make renderer's context current for reading texture data
    if (!this->renderer->ctx) {
        QMessageBox::warning(this, "Export Error",
                             "Renderer context not initialized.");
        return;
    }

    // Store the previous context to restore it later
    QOpenGLContext* previousContext = QOpenGLContext::currentContext();
    QSurface* previousSurface =
        previousContext ? previousContext->surface() : nullptr;

    // Make renderer context current
    this->renderer->ctx->makeCurrent(this->renderer->surface);

    // Create exporter
    Exporter exporter;

    // Export each output node
    int successCount = 0;
    int failCount = 0;

    for (auto& node : outputNodes) {
        // Get the output name from the node's property
        QString outputName = "";
        if (node->hasProp("name")) {
            outputName = node->getProp("name")->getValue().toString();
        }

        // Use node title if name property is empty
        if (outputName.isEmpty()) {
            outputName = "output";
        }

        // Get precision setting (0 = 8-bit, 1 = 16-bit)
        int precision = 0;
        if (node->hasProp("precision")) {
            precision = node->getProp("precision")->getValue().toInt();
        }

        // Get components setting (0=RGBA, 1=RGB, 2=R, 3=G, 4=B, 5=A)
        int components = 0;
        if (node->hasProp("components")) {
            components = node->getProp("components")->getValue().toInt();
        }

        // Generate filename using pattern
        QString filename = pattern;
        filename.replace("${project}", this->project->name);
        filename.replace("${name}", outputName);
        filename += ".png";

        QString fullPath = destination + "/" + filename;

        // Get the texture from the node
        if (!node->texture) {
            qDebug() << "Node" << node->title << "has no texture";
            failCount++;
            continue;
        }

        // Export using Exporter class
        ExportResult result = exporter.exportTexture(node->texture, fullPath,
                                                     precision, components);

        if (result.success) {
            successCount++;
        }
        else {
            failCount++;
        }
    }

    // Restore previous context
    if (previousContext && previousSurface) {
        previousContext->makeCurrent(previousSurface);
    }
    else {
        this->renderer->ctx->doneCurrent();
    }

    // Show result message
    QString message =
        QString("Export complete!\n\n") +
        QString("Successfully exported: %1 file(s)\n").arg(successCount);

    if (failCount > 0) {
        message += QString("Failed: %1 file(s)").arg(failCount);
    }

    QMessageBox::information(this, "Export", message);
}

MainWindow::~MainWindow()
{
    // Clean up renderer
    if (this->renderer) {
        delete this->renderer;
        this->renderer = nullptr;
    }
}