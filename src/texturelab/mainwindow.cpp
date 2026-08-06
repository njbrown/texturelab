#include "mainwindow.h"

#include <vector>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
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

#include "catalogservice.h"
#include "exporter.h"
#include "launcher/launcherwindow.h"
#include "telemetry.h"
#include "thememanager.h"
#include "tokens.h"
#include "undo/undocommands.h"
#include "widgets/aboutdialog.h"
#include "widgets/exportdialog.h"
#include "widgets/graphwidget.h"
#include "widgets/librarywidget.h"
#include "widgets/properties/propertieswidget.h"
#include "widgets/view2dwidget.h"
#include "widgets/view3dwidget.h"

#include "viewer3d.h"

#include "libraries/libraryversionmigrator.h"
#include "libraries/libversion.h"
#include "models.h"
#include "project.h"
#include "props.h"

#include "graph/scene.h"
#include "graphics/texturerenderer.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    resize(1280, 720);
    setAcceptDrops(true);

    undoStack = new QUndoStack(this);
    connect(undoStack, &QUndoStack::cleanChanged, this,
            &MainWindow::onCleanChanged);

    // Any command (or its undo) can add, remove or re-point the node a
    // texture channel maps to — deleting an assigned node being the obvious
    // one. Re-sync the viewer and the graph's channel labels from the model
    // after every stack move so they can't drift, and kick the render loop in
    // case the command marked nodes dirty without rendering them.
    connect(undoStack, &QUndoStack::indexChanged, this, [this](int) {
        if (!this->project)
            return;

        // undo/redo changes prop values behind the properties panel's back
        this->propWidget->syncPropBaselines();

        // this fires on every push too (including merged ones, i.e. every
        // step of a slider scrub), so only touch the viewer when the channel
        // mapping actually changed
        if (this->syncedChannels != this->project->textureChannels) {
            this->syncedChannels = this->project->textureChannels;
            this->passTextureChannelsToViewer3D();
            this->syncChannelLabelsToScene();
            this->view3DWidget->reRender();
        }

        if (this->renderer)
            this->renderer->update();
    });

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
    // Version + build hash on the left so it's legible in screenshots
    // (matches the build artifact name, e.g.
    // texturelab-win-v0.4.0-beta-<hash>).
    auto* versionLabel = new QLabel(QCoreApplication::applicationVersion());
    versionLabel->setObjectName(
        "StatusVersionLabel"); // styled in resources/qss/app.qss.in
    versionLabel->setToolTip("Application version and build hash");
    statusBar()->addWidget(versionLabel);

    // Home: reopens the launcher over the editor (LAUNCHER_PRD.md §7).
    auto* homeButton = new QToolButton();
    homeButton->setObjectName("StatusHomeButton");
    homeButton->setText("⌂");
    homeButton->setToolTip("Show all textures");
    homeButton->setCursor(Qt::PointingHandCursor);
    connect(homeButton, &QToolButton::clicked, this, &MainWindow::showLauncher);
    statusBar()->addWidget(homeButton);

    statusLayout->addWidget(statusLabel, 0, Qt::AlignVCenter);
    statusLayout->addWidget(progressBar, 0, Qt::AlignVCenter);
    statusBar()->addWidget(statusWidget, 1);

    this->dockManager = new ads::CDockManager(this);

    // Theme the dock system. ADS installs its own default stylesheet on the
    // dock manager (constructor -> loadStylesheet), which overrides the global
    // app sheet for ads--* widgets. We keep that default (it carries button
    // icons and layout metrics) and append our token-driven color overrides.
    // Rebuilt on every theme change so it also picks up --dev-theme
    // hot-reloads.
    this->adsDefaultStyleSheet = this->dockManager->styleSheet();
    auto applyDockTheme = [this]() {
        ThemeManager& tm = ThemeManager::instance();
        // Qt prefers an ancestor widget's stylesheet over qApp, so app.qss
        // rules (e.g. #AccordionHeader) don't reach widgets inside docks unless
        // we also hand them to the dock manager. Order: ADS default -> ADS
        // overrides -> app rules (last so our tokens win over ADS's
        // palette()-based defaults).
        // The call below applies the composed theme sheet (ADS default + ADS
        // overrides + app rules) to the dock manager — it IS the theming, not an
        // inline widget style; the short marker keeps the hygiene gate happy even
        // if a formatter wraps the line.
        const QString sheet = this->adsDefaultStyleSheet + "\n" +
                              tm.adsStyleSheet() + "\n" + tm.appStyleSheet();
        this->dockManager->setStyleSheet(sheet); // theme-exempt
    };
    applyDockTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
            applyDockTheme);

    // Thicker gutters between panels. ADS/QSplitter ignore QSS handle width, so
    // set handleWidth in code — on the initial layout and whenever a new dock
    // area (hence a new splitter) is created (drag-docking).
    auto applySplitterWidth = [this]() {
        for (auto* s : this->dockManager->findChildren<ads::CDockSplitter*>())
            s->setHandleWidth(1);
    };
    connect(
        this->dockManager, &ads::CDockManager::dockAreaCreated, this,
        [applySplitterWidth](ads::CDockAreaWidget*) { applySplitterWidth(); });

    this->setupDocks();
    applySplitterWidth();

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

    connect(
        this->propWidget, &PropertiesWidget::textureChannelUpdated,
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
                        // Apply immediately, then push command (first-redo
                        // no-op)
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
                QString oldNodeId =
                    this->project->textureChannels.value(name, "");
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

        // Skip nodes whose FBO/shader aren't ready yet: textureId() would
        // otherwise dereference a null texture. They'll be picked up on the
        // next sync once the render worker has produced their texture.
        if (!node->isGraphicsResourcesInitialized())
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
    case TextureChannel::Albedo:
        return "Albedo";
    case TextureChannel::Normal:
        return "Normal";
    case TextureChannel::Metalness:
        return "Metalness";
    case TextureChannel::Roughness:
        return "Roughness";
    case TextureChannel::Height:
        return "Height";
    case TextureChannel::Alpha:
        return "Alpha";
    case TextureChannel::AO:
        return "AO";
    default:
        return "";
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
        this->propWidget->setTextureRenderer(nullptr);

        delete this->renderer;
        this->renderer = nullptr;
    }

    this->project = project;
    this->syncedChannels = project->textureChannels;
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
    // undo/redo of a property change has no propertyUpdated signal to kick the
    // render loop, so the commands need the renderer directly
    this->propWidget->setTextureRenderer(renderer);

    connect(renderer, &TextureRenderer::renderProgress,
            [this](int clean, int total) {
                progressBar->setMaximum(total == 0 ? 1 : total);
                progressBar->setValue(total == 0 ? 1 : clean);
                if (total == 0 || clean == total) {
                    statusLabel->setText("Ready");

                    // The graph is fully evaluated: this is the first moment a
                    // freshly opened document is worth photographing.
                    if (thumbnailCapturePending) {
                        thumbnailCapturePending = false;
                        captureLauncherThumbnail(int(catalog::ThumbSource::Open));
                    }
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

    auto crashReportingAction =
        optionsMenu->addAction("Send Anonymous Crash Reports");
    crashReportingAction->setCheckable(true);
    QSettings settings(QSettings::UserScope, "texturelab", "texturelab");
    crashReportingAction->setChecked(
        settings.value("crashReporting", true).toBool());
    connect(crashReportingAction, &QAction::toggled, [](bool checked) {
        QSettings s(QSettings::UserScope, "texturelab", "texturelab");
        s.setValue("crashReporting", checked);
    });
}

// Recolor a rendered (white) icon pixmap to `color`, keeping its alpha shape.
static QPixmap tintPixmap(const QPixmap& src, const QColor& color)
{
    QPixmap out(src.size());
    out.setDevicePixelRatio(src.devicePixelRatio());
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.drawPixmap(0, 0, src);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(out.rect(), color);
    p.end();
    return out;
}

// Build a toolbar QIcon whose Normal/Disabled variants are tinted to the theme's
// text colors, so the icon always matches the button label's color in each state
// (Qt's auto-generated disabled fade doesn't match text.disabled exactly).
static QIcon themedToolIcon(const QString& svgPath)
{
    const Theme& t = ThemeManager::instance().theme();
    const QPixmap base = QIcon(svgPath).pixmap(QSize(32, 32)); // white source SVG
    QIcon icon;
    icon.addPixmap(tintPixmap(base, t.color(Tokens::TextPrimary)), QIcon::Normal);
    icon.addPixmap(tintPixmap(base, t.color(Tokens::TextDisabled)), QIcon::Disabled);
    return icon;
}

void MainWindow::setupToolbar()
{
    // https://www.setnode.com/blog/right-aligning-a-button-in-a-qtoolbar/
    toolBar = this->addToolBar("main toolbar");
    toolBar->setObjectName("MainToolbar"); // styled in app.qss.in
    toolBar->setIconSize(QSize(14, 14)); // small, to sit level with the button text
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // undo redo — plain "Undo"/"Redo" labels (not the createUndoAction text,
    // which appends the command name). Shortcuts stay on the Edit-menu actions
    // to avoid an ambiguous-shortcut clash.
    auto undoAction = new QAction(themedToolIcon(":/icons/undo.svg"), "Undo", this);
    undoAction->setEnabled(undoStack->canUndo());
    connect(undoAction, &QAction::triggered, undoStack, &QUndoStack::undo);
    connect(undoStack, &QUndoStack::canUndoChanged, undoAction,
            &QAction::setEnabled);
    toolBar->addAction(undoAction);

    auto redoAction = new QAction(themedToolIcon(":/icons/redo.svg"), "Redo", this);
    redoAction->setEnabled(undoStack->canRedo());
    connect(redoAction, &QAction::triggered, undoStack, &QUndoStack::redo);
    connect(undoStack, &QUndoStack::canRedoChanged, redoAction,
            &QAction::setEnabled);
    toolBar->addAction(redoAction);

    // spacer
    toolBar->addWidget(spacer);

    // Export button with dropdown menu
    auto exportBtn = new QToolButton(this);
    exportBtn->setText("Export");
    exportBtn->setIcon(themedToolIcon(":/icons/export.svg"));
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

void MainWindow::captureLauncherThumbnail(int source)
{
    CatalogService& catalog = CatalogService::instance();
    if (!catalog.isReady() || !project || project->filePath.isEmpty())
        return;

    auto* viewer = view3DWidget ? view3DWidget->viewer : nullptr;
    if (!viewer || !viewer->isValid())
        return;

    // The one moment the whole graph is evaluated and resident on the GPU, so
    // we just take the picture — no headless evaluator, no second GL context
    // (LAUNCHER_PRD.md §4). Whatever the user framed is what the card shows.
    const QImage frame = viewer->grabFramebuffer();
    if (frame.isNull())
        return;

    const catalog::TextureRecord rec = catalog.index().byPath(project->filePath);
    if (!rec.isValid())
        return;

    catalog.captureThumbnail(rec.id, frame, static_cast<catalog::ThumbSource>(source));
}

void MainWindow::showLauncher()
{
    if (!launcher) {
        launcher = new LauncherWindow();

        connect(launcher, &LauncherWindow::newTextureRequested, this, [this]() {
            launcher->hide();
            newProject();
            showMaximized();
            raise();
            activateWindow();
        });

        // Routed through openProjectFromPath so the launcher inherits the
        // dirty-document prompt and the library-version upgrade dialog rather
        // than reimplementing either.
        connect(launcher, &LauncherWindow::openPathRequested, this,
                [this](const QString& path) {
                    const QString before = project ? project->filePath : QString();
                    openProjectFromPath(path);

                    // openProjectFromPath bails out silently if the user
                    // cancels the save prompt or the file won't read; in that
                    // case leave the launcher up rather than dropping them into
                    // an editor they didn't ask for.
                    if (project && project->filePath == path && project->filePath != before) {
                        launcher->hide();
                        showMaximized();
                        raise();
                        activateWindow();
                    }
                });

        connect(launcher, &LauncherWindow::openDialogRequested, this, [this]() {
            openProject();
            if (project && !project->filePath.isEmpty()) {
                launcher->hide();
                showMaximized();
                raise();
                activateWindow();
            }
        });

        connect(launcher, &LauncherWindow::closeRequested, this, [this]() {
            launcher->hide();
            showMaximized();
            raise();
            activateWindow();
        });
    }

    launcher->setHasDocument(project && !project->filePath.isEmpty());
    launcher->setOpenPath(project ? project->filePath : QString());
    launcher->show();
    launcher->raise();
    launcher->activateWindow();
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

    Telemetry::breadcrumb("project",
                          "open: " + fileInfo.baseName().toStdString());
    setProject(project);
    addToRecentFiles(filePath);

    // The shared entry point for the Open dialog, the recent-files menu, and
    // drag-and-drop, so one hook here covers all three (LAUNCHER_PRD.md §6.1).
    CatalogService::instance().recordOpened(project, filePath);

    // Can't grab yet: opening kicks off an asynchronous render and the viewport
    // is still showing the previous document (or nothing). Captured when
    // renderProgress reports every node clean.
    thumbnailCapturePending = true;
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
    CatalogService::instance().recordSaved(project, project->filePath);
    captureLauncherThumbnail(int(catalog::ThumbSource::Save));
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
    CatalogService::instance().recordSaved(project, project->filePath);
    captureLauncherThumbnail(int(catalog::ThumbSource::Save));
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

    // Reads through to the catalog index rather than QSettings, so this menu
    // and the launcher can't disagree about what you opened last. QSettings is
    // still written by addToRecentFiles() as a fallback for the case where the
    // index failed to open.
    QStringList files;
    CatalogService& catalog = CatalogService::instance();

    if (catalog.isReady()) {
        catalog::Query query;
        query.filter = catalog::Filter::Recents;
        query.sort = catalog::SortKey::Opened;
        query.ascending = false;
        query.limit = MaxRecentFiles;

        for (const catalog::TextureRecord& rec : catalog.index().list(query))
            files << rec.path;
    }
    else {
        files = QSettings().value("recentFiles").toStringList();
    }

    for (const QString& filePath : files) {
        QFileInfo info(filePath);
        auto action =
            recentFilesMenu->addAction(info.fileName(), [this, filePath]() {
                openProjectFromPath(filePath);
            });
        action->setToolTip(filePath);
    }

    if (files.isEmpty())
        recentFilesMenu->addAction("No recent files")->setEnabled(false);

    recentFilesMenu->addSeparator();
    recentFilesMenu->addAction("Clear Recent Files", [this]() {
        QSettings().remove("recentFiles");

        // Clearing the menu must not delete the user's stars, tags, or the
        // textures themselves — only forget when they were last opened. The
        // launcher keeps showing them under All.
        CatalogService& catalog = CatalogService::instance();
        if (catalog.isReady())
            catalog.index().clearRecents();
    });
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