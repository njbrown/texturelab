#include "mainwindow.h"

#include <vector>

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

#include "DockAreaWidget.h"
#include "DockSplitter.h"

#include "exporter.h"
#include "telemetry.h"
#include "undo/undocommands.h"
#include "widgets/aboutdialog.h"
#include "widgets/exportdialog.h"
#include "widgets/graphwidget.h"
#include "widgets/librarywidget.h"
#include "widgets/properties/propertieswidget.h"
#include "widgets/view2dwidget.h"
#include "widgets/view3dwidget.h"

#include "viewer3d.h"

#include "models.h"
#include "libraries/libraryversionmigrator.h"
#include "libraries/libversion.h"
#include "project.h"
#include "props.h"

#include "graphics/texturerenderer.h"
#include "graph/scene.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    resize(1280, 720);
    setAcceptDrops(true);

    undoStack = new QUndoStack(this);
    connect(undoStack, &QUndoStack::cleanChanged, this, &MainWindow::onCleanChanged);

    this->setupMenus();
    this->setupToolbar();

    this->renderer = nullptr;
    this->exportDialog = nullptr;

    statusLabel = new QLabel("Ready");
    progressBar = new QProgressBar();
    progressBar->setRange(0, 1);
    progressBar->setValue(1);
    progressBar->setFixedWidth(180);
    progressBar->setTextVisible(false);

    auto* statusWidget = new QWidget();
    auto* statusLayout = new QHBoxLayout(statusWidget);
    // statusLayout->setContentsMargins(4, 0, 4, 0);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(6);
    statusLayout->addStretch();
    statusLayout->addWidget(statusLabel, 0, Qt::AlignVCenter);
    statusLayout->addWidget(progressBar, 0, Qt::AlignVCenter);
    statusBar()->addWidget(statusWidget, 1);

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

    connect(this->graphWidget, &GraphWidget::frameSelectionChanged,
            [this](const FramePtr& frame) {
                if (!!frame) {
                    this->propWidget->setSelectedFrame(frame);
                }
                else {
                    this->propWidget->clearSelection();
                }
            });

    connect(this->graphWidget, &GraphWidget::commentSelectionChanged,
            [this](const CommentPtr& comment) {
                if (!!comment) {
                    this->propWidget->setSelectedComment(comment);
                }
                else {
                    this->propWidget->clearSelection();
                }
            });

    connect(this->propWidget, &PropertiesWidget::framePropertyChanged,
            [this](const FramePtr& frame) {
                this->graphWidget->syncFrameToScene(frame);
            });

    connect(this->propWidget, &PropertiesWidget::commentPropertyChanged,
            [this](const CommentPtr& comment) {
                this->graphWidget->syncCommentToScene(comment);
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
                if (!this->project)
                    return;

                auto syncViewer = [this]() {
                    this->passTextureChannelsToViewer3D();
                    this->syncChannelLabelsToScene();
                    this->view3DWidget->reRender();
                    if (this->renderer)
                        this->renderer->update();
                };

                if (name == TextureChannel::None) {
                    // Unassign this node from whichever channel it's in
                    for (auto ch : this->project->textureChannels.keys()) {
                        if (this->project->textureChannels[ch] == node->id) {
                            QString oldNodeId = node->id;
                            // Apply immediately, then push command (first-redo no-op)
                            this->project->textureChannels.remove(ch);
                            syncViewer();
                            if (undoStack)
                                undoStack->push(new TextureChannelAssignCommand(
                                    this->project, ch, oldNodeId, "", syncViewer));
                            break;
                        }
                    }
                }
                else {
                    QString oldNodeId = this->project->textureChannels.value(name, "");
                    // Apply immediately, then push command (first-redo no-op)
                    this->project->textureChannels[name] = node->id;
                    syncViewer();
                    if (undoStack)
                        undoStack->push(new TextureChannelAssignCommand(
                            this->project, name, oldNodeId, node->id, syncViewer));
                }
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
        case TextureChannel::Alpha:
            viewer->setAlphaTexture(node->textureId());
            break;
        default:
            break;
        }
    }
}

static QString channelName(TextureChannel ch)
{
    switch (ch) {
    case TextureChannel::Albedo:    return "Albedo";
    case TextureChannel::Normal:    return "Normal";
    case TextureChannel::Metalness: return "Metalness";
    case TextureChannel::Roughness: return "Roughness";
    case TextureChannel::Height:    return "Height";
    case TextureChannel::Alpha:     return "Alpha";
    case TextureChannel::AO:        return "AO";
    default:                        return "";
    }
}

void MainWindow::syncChannelLabelsToScene()
{
    if (!project || !graphWidget->scene)
        return;

    // clear all labels first
    for (auto& node : graphWidget->scene->nodes)
        node->setChannel("");

    // set labels from project state
    for (auto ch : project->textureChannels.keys()) {
        auto nodeId = project->textureChannels[ch];
        auto sceneNode = graphWidget->scene->nodes.value(nodeId);
        if (sceneNode)
            sceneNode->setChannel(channelName(ch));
    }
}

void MainWindow::setProject(TextureProjectPtr project)
{
    // Clear widget state from the old project
    this->view2DWidget->clearSelection();
    this->view3DWidget->viewer->clearTextures();
    this->view3DWidget->reRender();

    // Clean up old renderer before creating new one
    if (this->renderer) {
        this->graphWidget->setTextureRenderer(nullptr);
        this->view2DWidget->setTextureRenderer(nullptr);

        delete this->renderer;
        this->renderer = nullptr;
    }

    this->project = project;
    this->graphWidget->setTextureProject(project);
    this->syncChannelLabelsToScene();
    this->libraryWidget->setLibrary(project->library);
    this->libraryWidget->setLibraryVersion(
        project->libraryVersion,
        project->libraryVersion == libVersionToString(currentLibVersion()));

    this->propWidget->clearSelection();
    this->propWidget->setProject(project);
    this->propWidget->setScene(this->graphWidget->scene);

    renderer = new TextureRenderer();
    renderer->setProject(project);
    this->graphWidget->setTextureRenderer(renderer);
    this->view2DWidget->setTextureRenderer(renderer);

    connect(renderer, &TextureRenderer::renderProgress,
            [this](int clean, int total) {
                progressBar->setMaximum(total == 0 ? 1 : total);
                progressBar->setValue(total == 0 ? 1 : clean);
                if (total == 0 || clean == total) {
                    statusLabel->setText("Ready");
                }
                else {
                    statusLabel->setText(
                        QString("Rendering %1 / %2").arg(clean).arg(total));
                }
            });

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
                        case TextureChannel::Alpha:
                            viewer->setAlphaTexture(texId);
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

    setWindowTitle(project->name + " - TextureLab");
    undoStack->clear();
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

    recentFilesMenu = fileMenu->addMenu("Open Recent");
    connect(recentFilesMenu, &QMenu::aboutToShow, this,
            &MainWindow::updateRecentFilesMenu);

    fileMenu->addSeparator();
    fileMenu->addAction("Edit", []() {});

    auto editMenu = this->menuBar()->addMenu("Edit");
    auto undoAction = undoStack->createUndoAction(this, tr("Undo"));
    undoAction->setShortcut(QKeySequence::Undo);
    editMenu->addAction(undoAction);
    auto redoAction = undoStack->createRedoAction(this, tr("Redo"));
    redoAction->setShortcut(QKeySequence::Redo);
    editMenu->addAction(redoAction);
    editMenu->addAction("Cut", [=]() { graphWidget->executeCut(); });
    editMenu->addAction("Copy", [=]() { graphWidget->executeCopy(); });
    editMenu->addAction("Paste", [=]() { graphWidget->executePaste(); });

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
            if (!promptSaveIfDirty())
                return;
            auto project = Project::loadTexture(":examples/" + example);
            QString projectName = example;
            projectName.replace(".texture", "");
            project->name = projectName;
            setProject(project);
        });
    }

    auto optionsMenu = this->menuBar()->addMenu("Help");
    optionsMenu->addAction("About", [this]() {
        AboutDialog dialog(this);
        dialog.exec();
    });

    optionsMenu->addSeparator();

    auto crashReportingAction = optionsMenu->addAction("Send Anonymous Crash Reports");
    crashReportingAction->setCheckable(true);
    QSettings settings(QSettings::UserScope, "texturelab", "texturelab");
    crashReportingAction->setChecked(settings.value("crashReporting", true).toBool());
    connect(crashReportingAction, &QAction::toggled, [](bool checked) {
        QSettings s(QSettings::UserScope, "texturelab", "texturelab");
        s.setValue("crashReporting", checked);
    });
}

void MainWindow::setupToolbar()
{
    // https://www.setnode.com/blog/right-aligning-a-button-in-a-qtoolbar/
    toolBar = this->addToolBar("main toolbar");

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // undo redo — reuse the same actions wired to the stack
    toolBar->addAction(undoStack->createUndoAction(this));
    toolBar->addAction(undoStack->createRedoAction(this));

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
    this->graphWidget->setUndoStack(undoStack);
    this->view2DWidget = new View2DWidget();
    this->view3DWidget = new View3DWidget();

    auto graphArea =
        addDock("Graph", ads::CenterDockWidgetArea, graphWidget, nullptr);
    auto leftArea = addDock("2D View", ads::LeftDockWidgetArea,
                            this->view2DWidget, graphArea);

    this->propWidget = new PropertiesWidget();
    this->propWidget->setUndoStack(undoStack);
    auto rightArea = addDock("Properties", ads::RightDockWidgetArea,
                             this->propWidget, graphArea);

    this->libraryWidget = new LibraryWidget();
    connect(this->libraryWidget, &LibraryWidget::upgradeRequested, this,
            &MainWindow::upgradeCurrentProjectLibrary);
    setWidgetRatiosInArea(graphArea, {1.0f / 5, 3.0f / 5, 1.0f / 5});

    addDock("3D View", ads::BottomDockWidgetArea, this->view3DWidget, leftArea);
    addDock("Library", ads::BottomDockWidgetArea, this->libraryWidget,
            rightArea);
    setWidgetRatiosInArea(leftArea, {0.5f, 0.5f});
    setWidgetRatiosInArea(rightArea, {0.5f, 0.5f});

    QTimer::singleShot(0, this, [this]() { graphWidget->setFocus(); });
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
    if (!promptSaveIfDirty())
        return;

    auto filePath = QFileDialog::getOpenFileName(this, "Open Texture File", "",
                                                 "Texturelab File (*.texture)");

    if (filePath.isNull() || filePath.isEmpty())
        return;

    openProjectFromPath(filePath);
}

void MainWindow::openProjectFromPath(const QString& filePath)
{
    if (!promptSaveIfDirty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Open Texture",
                             "Could not open file:\n" + filePath);
        return;
    }
    auto json = QJsonDocument::fromJson(file.readAll()).object();
    file.close();

    LibraryVersionMigrator migrator(json);
    if (migrator.needsMigration()) {
        QStringList chain;
        chain << libVersionToString(migrator.sourceVersion());
        for (auto v : migrator.versionsCrossed())
            chain << libVersionToString(v);

        auto choice = QMessageBox::question(
            this, "Upgrade Texture?",
            QString("This texture was created with library version %1.\n\n"
                    "Upgrade it to %2 (%3) to use the latest nodes and "
                    "improvements? A few node behaviors may change slightly.")
                .arg(libVersionToString(migrator.sourceVersion()),
                     libVersionToString(migrator.targetVersion()),
                     chain.join(" → ")),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (choice == QMessageBox::Yes)
            json = migrator.migrate();
    }

    auto project = Project::loadTextureFromJson(json);

    QFileInfo fileInfo(filePath);
    project->name = fileInfo.baseName();
    project->filePath = filePath;

    Telemetry::breadcrumb("project", "open: " + fileInfo.baseName().toStdString());
    setProject(project);
    addToRecentFiles(filePath);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    for (const auto& url : event->mimeData()->urls()) {
        if (url.isLocalFile() &&
            url.toLocalFile().endsWith(".texture", Qt::CaseInsensitive)) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    for (const auto& url : event->mimeData()->urls()) {
        if (!url.isLocalFile())
            continue;

        auto filePath = url.toLocalFile();
        if (!filePath.endsWith(".texture", Qt::CaseInsensitive))
            continue;

        event->acceptProposedAction();
        openProjectFromPath(filePath);
        return;
    }

    event->ignore();
}

void MainWindow::upgradeCurrentProjectLibrary()
{
    if (!this->project)
        return;

    // Round-trip through the same JSON the file format uses, so the live
    // "Upgrade" button in the Library dock goes through the exact same
    // pure-JSON migration path as opening a legacy file does.
    auto bytes = Project::saveTexture(this->project);
    auto json = QJsonDocument::fromJson(bytes).object();

    LibraryVersionMigrator migrator(json);
    if (!migrator.needsMigration())
        return;

    auto choice = QMessageBox::warning(
        this, "Upgrade Library Version?",
        "Upgrading the library version is irreversible and clears the "
        "undo/redo history for this session.\n\n"
        "Save your project (or save a copy) first if you want to keep the "
        "ability to go back to the current version.\n\n"
        "Continue with the upgrade?",
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

    if (choice != QMessageBox::Yes)
        return;

    auto newProject = Project::loadTextureFromJson(migrator.migrate());
    newProject->name = this->project->name;
    newProject->filePath = this->project->filePath;

    setProject(newProject);
}

void MainWindow::newProject()
{
    if (!promptSaveIfDirty())
        return;
    Telemetry::breadcrumb("project", "new project");
    setProject(TextureProject::createEmpty());
}

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

    graphWidget->syncPositionsToModel();

    Telemetry::breadcrumb("project", "save: " + project->name.toStdString());
    QFile file(project->filePath);
    file.open(QIODevice::WriteOnly);
    file.write(Project::saveTexture(project));
    file.close();
    undoStack->setClean();
    addToRecentFiles(project->filePath);
}

void MainWindow::saveProjectAs()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, "Save Texture As...", QString(), "Texturelab File (*.texture)");

    if (filePath.isNull() || filePath.isEmpty())
        return;

    if (!filePath.endsWith(".texture", Qt::CaseInsensitive))
        filePath += ".texture";

    project->filePath = filePath;

    QFileInfo fileInfo(filePath);
    project->name = fileInfo.baseName();

    graphWidget->syncPositionsToModel();

    QFile file(project->filePath);
    file.open(QIODevice::WriteOnly);
    file.write(Project::saveTexture(project));
    file.close();
    undoStack->setClean();
    setWindowTitle(project->name + " - TextureLab");
    addToRecentFiles(project->filePath);
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
    Telemetry::breadcrumb("project", "export to: " + destination.toStdString());
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

void MainWindow::addToRecentFiles(const QString& filePath)
{
    QSettings settings;
    QStringList files = settings.value("recentFiles").toStringList();
    files.removeAll(filePath);
    files.prepend(filePath);
    while (files.size() > MaxRecentFiles)
        files.removeLast();
    settings.setValue("recentFiles", files);
}

void MainWindow::updateRecentFilesMenu()
{
    recentFilesMenu->clear();

    QSettings settings;
    QStringList files = settings.value("recentFiles").toStringList();

    for (const QString& filePath : files) {
        QFileInfo info(filePath);
        auto action = recentFilesMenu->addAction(
            info.fileName(), [this, filePath]() { openProjectFromPath(filePath); });
        action->setToolTip(filePath);
    }

    if (files.isEmpty())
        recentFilesMenu->addAction("No recent files")->setEnabled(false);

    recentFilesMenu->addSeparator();
    recentFilesMenu->addAction("Clear Recent Files",
                               [this]() { QSettings().remove("recentFiles"); });
}

void MainWindow::onCleanChanged(bool clean)
{
    if (!project)
        return;
    QString title = project->name + " - TextureLab";
    setWindowTitle(clean ? title : "*" + title);
}

bool MainWindow::promptSaveIfDirty()
{
    if (undoStack->isClean())
        return true;

    QString name = project ? project->name : "Untitled";
    auto choice = QMessageBox::question(
        this, "Unsaved Changes",
        QString("Save changes to \"%1\" before continuing?").arg(name),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (choice == QMessageBox::Save) {
        saveProject();
        return undoStack->isClean(); // false if save was cancelled
    }
    return choice == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!promptSaveIfDirty()) {
        event->ignore();
        return;
    }
    event->accept();
}

MainWindow::~MainWindow()
{
    if (this->renderer) {
        delete this->renderer;
        this->renderer = nullptr;
    }
}