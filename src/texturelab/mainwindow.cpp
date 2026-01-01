#include "mainwindow.h"

#include <vector>

#include <QDebug>
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
                        // todo: clear channel
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
        }
    }
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
        filename.replace("${project}",
                         "untitled"); // TODO: use actual project name
        filename.replace("${name}", outputName);
        filename += ".png";

        QString fullPath = destination + "/" + filename;

        // Get the texture from the node
        if (!node->texture) {
            qDebug() << "Node" << node->title << "has no texture";
            failCount++;
            continue;
        }

        int width = node->texture->width();
        int height = node->texture->height();

        // Bind the FBO and read pixels
        node->texture->bind();
        QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();

        // Read as float data (since texture is GL_RGBA32F)
        std::vector<float> floatData(width * height * 4);
        gl->glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT,
                         floatData.data());
        node->texture->release();

        QImage img;

        // Determine number of channels for output
        int outputChannels = 4;
        if (components == 1)
            outputChannels = 3; // RGB
        else if (components >= 2 && components <= 5)
            outputChannels = 1; // Single channel

        if (precision == 1) {
            // 16-bit precision
            if (outputChannels == 1) {
                // Grayscale 16-bit
                img = QImage(width, height, QImage::Format_Grayscale16);

                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int idx = (y * width + x) * 4;
                        float value = 0.0f;

                        // Extract the selected channel
                        if (components == 2)
                            value = floatData[idx + 0]; // Red
                        else if (components == 3)
                            value = floatData[idx + 1]; // Green
                        else if (components == 4)
                            value = floatData[idx + 2]; // Blue
                        else if (components == 5)
                            value = floatData[idx + 3]; // Alpha

                        // Convert to 16-bit (0-65535)
                        quint16 val16 = qBound(
                            0, static_cast<int>(value * 65535.0f), 65535);
                        img.setPixel(x, y, qGray(val16, val16, val16));
                    }
                }
            }
            else if (outputChannels == 3) {
                // RGB 16-bit
                img = QImage(width, height, QImage::Format_RGBX64);

                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int idx = (y * width + x) * 4;

                        quint16 r = qBound(
                            0, static_cast<int>(floatData[idx + 0] * 65535.0f),
                            65535);
                        quint16 g = qBound(
                            0, static_cast<int>(floatData[idx + 1] * 65535.0f),
                            65535);
                        quint16 b = qBound(
                            0, static_cast<int>(floatData[idx + 2] * 65535.0f),
                            65535);

                        img.setPixelColor(x, y,
                                          QColor::fromRgb(r, g, b, 65535));
                    }
                }
            }
            else {
                // RGBA 16-bit
                img = QImage(width, height, QImage::Format_RGBA64);

                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int idx = (y * width + x) * 4;

                        quint16 r = qBound(
                            0, static_cast<int>(floatData[idx + 0] * 65535.0f),
                            65535);
                        quint16 g = qBound(
                            0, static_cast<int>(floatData[idx + 1] * 65535.0f),
                            65535);
                        quint16 b = qBound(
                            0, static_cast<int>(floatData[idx + 2] * 65535.0f),
                            65535);
                        quint16 a = qBound(
                            0, static_cast<int>(floatData[idx + 3] * 65535.0f),
                            65535);

                        img.setPixelColor(x, y, QColor::fromRgba64(r, g, b, a));
                    }
                }
            }
        }
        else {
            // 8-bit precision
            if (outputChannels == 1) {
                // Grayscale 8-bit
                img = QImage(width, height, QImage::Format_Grayscale8);

                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int idx = (y * width + x) * 4;
                        float value = 0.0f;

                        // Extract the selected channel
                        if (components == 2)
                            value = floatData[idx + 0]; // Red
                        else if (components == 3)
                            value = floatData[idx + 1]; // Green
                        else if (components == 4)
                            value = floatData[idx + 2]; // Blue
                        else if (components == 5)
                            value = floatData[idx + 3]; // Alpha

                        // Convert to 8-bit (0-255)
                        quint8 val8 =
                            qBound(0, static_cast<int>(value * 255.0f), 255);
                        img.setPixel(x, y, qGray(val8, val8, val8));
                    }
                }
            }
            else if (outputChannels == 3) {
                // RGB 8-bit
                img = QImage(width, height, QImage::Format_RGB888);

                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int idx = (y * width + x) * 4;

                        quint8 r = qBound(
                            0, static_cast<int>(floatData[idx + 0] * 255.0f),
                            255);
                        quint8 g = qBound(
                            0, static_cast<int>(floatData[idx + 1] * 255.0f),
                            255);
                        quint8 b = qBound(
                            0, static_cast<int>(floatData[idx + 2] * 255.0f),
                            255);

                        img.setPixelColor(x, y, QColor(r, g, b));
                    }
                }
            }
            else {
                // RGBA 8-bit - use the simple toImage() conversion
                img = node->texture->toImage();
            }
        }

        // Flip image vertically (OpenGL reads bottom-to-top)
        img = img.mirrored(false, true);

        // Save image
        if (img.save(fullPath)) {
            qDebug() << "Exported:" << fullPath;
            successCount++;
        }
        else {
            qDebug() << "Failed to save:" << fullPath;
            failCount++;
        }
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

MainWindow::~MainWindow() {}