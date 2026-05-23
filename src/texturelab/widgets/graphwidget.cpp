#include "graphwidget.h"
#include <QCursor>
#include <QDragEnterEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QSignalBlocker>
#include <QToolBar>

#include "./graphics/texturerenderer.h"
#include "./models.h"
#include "./utils.h"
#include "graph/comment.h"
#include "graph/frame.h"
#include "graph/scene.h"
#include "libraries/library.h"
#include "librarywidget.h"
#include "nodegraph.h"
#include "nodesearchpopup.h"

GraphWidget::GraphWidget() : QMainWindow(nullptr)
{
    graph = new nodegraph::NodeGraph(this);
    this->setCentralWidget(graph);

    this->setAcceptDrops(true);

    setupToolbar();

    // Create search popup
    searchPopup = new NodeSearchPopup(this);
    connect(searchPopup, &NodeSearchPopup::itemSelected, this,
            &GraphWidget::addItemFromSearch);

    // Enable mouse tracking to capture cursor position
    setMouseTracking(true);
    graph->setMouseTracking(true);

    connect(graph, &nodegraph::NodeGraph::connectionAdded,
            [=](nodegraph::ConnectionPtr con) {
                qDebug() << "CONNECTION ADDED";

                // auto sceneCon = project->getConnectionById(con->id());
                // sceneCon->rightNode->isDirty = true;

                auto leftNode =
                    project->getNodeById(con->startPort->node->id());
                auto rightNode = project->getNodeById(con->endPort->node->id());
                auto rightName = con->endPort->name;

                project->addConnection(leftNode, rightNode, rightName);

                // make ready for update
                rightNode->isDirty = true;

                // todo: try to update later
                renderer->update();
            });

    connect(graph, &nodegraph::NodeGraph::connectionRemoved,
            [=](nodegraph::ConnectionPtr con) {
                qDebug() << "CONNECTION REMOVED";

                auto leftNodeId = con->startPort->node->id();
                auto rightNodeId = con->endPort->node->id();
                auto portName = con->endPort->name;

                auto removedCon = project->removeConnection(
                    leftNodeId, rightNodeId, portName);

                removedCon->rightNode->isDirty = true;

                // todo: try to update later
                renderer->update();
            });

    connect(graph, &nodegraph::NodeGraph::nodeSelectionChanged,
            [=](nodegraph::NodePtr node) {
                if (!!node) {
                    qDebug() << "NODE SELECTED";

                    if (!!project) {
                        auto texNode = project->getNodeById(node->id());
                        emit nodeSelectionChanged(texNode);
                    }
                }
                else {
                    qDebug() << "NODE DESELECTED";
                    emit nodeSelectionChanged(TextureNodePtr(nullptr));
                }
            });

    connect(graph, &nodegraph::NodeGraph::nodeDoubleClicked,
            [=](nodegraph::NodePtr node) {
                if (!!node) {
                    // qDebug() << "NODE DOUBLE CLICKED";

                    if (!!project) {
                        auto texNode = project->getNodeById(node->id());
                        emit nodeDoubleClicked(texNode);
                    }
                }
                else {
                    // qDebug() << "NODE DESELECTED";
                    emit nodeDoubleClicked(TextureNodePtr(nullptr));
                }
            });

    // connect(graph, &nodegraph::NodeGraph::nodeAdded,
    //         [=](nodegraph::NodePtr node) { qDebug() << "NODE ADDED"; });

    // connect(graph, &nodegraph::NodeGraph::nodeRemoved,
    //         [=](nodegraph::NodePtr node) { qDebug() << "NODE REMOVED"; });

    // library = nullptr;
}

void GraphWidget::setupToolbar()
{
    auto toolbar = this->addToolBar("Graph");
    toolbar->setMovable(false);

    toolbar->addWidget(new QLabel("Resolution: "));

    resolutionPicker = new QComboBox();
    for (int res : {32, 64, 128, 256, 512, 1024, 2048, 4096})
        resolutionPicker->addItem(QString("%1 x %1").arg(res), res);
    resolutionPicker->setCurrentIndex(5); // default: 1024
    resolutionPicker->setEnabled(false);
    toolbar->addWidget(resolutionPicker);

    connect(resolutionPicker, &QComboBox::currentIndexChanged, this,
            [=](int /*index*/) {
                if (!project)
                    return;
                int res = resolutionPicker->currentData().toInt();
                project->textureWidth = res;
                project->textureHeight = res;
                for (auto& node : project->nodes)
                    node->isDirty = true;
                if (renderer)
                    renderer->update();
            });

    toolbar->addSeparator();

    toolbar->addWidget(new QLabel("Seed: "));

    seedInput = new QSpinBox();
    seedInput->setMinimum(0);
    seedInput->setMaximum(99999);
    seedInput->setValue(0);
    seedInput->setEnabled(false);
    toolbar->addWidget(seedInput);

    connect(seedInput, &QSpinBox::valueChanged, this, [=]() {
        if (!project)
            return;
        project->randomSeed = seedInput->value();
        for (auto& node : project->nodes)
            node->isDirty = true;
        if (renderer)
            renderer->update();
    });
}

void GraphWidget::setTextureProject(TextureProjectPtr project)
{
    // generate nodes from texture project

    //   auto scene = graph->scene();
    auto scene = nodegraph::Scene::create();
    // auto scene = new nodegraph::Scene();
    this->scene = scene;
    this->project = project;

    // Update toolbar controls from project settings
    {
        QSignalBlocker b1(resolutionPicker);
        QSignalBlocker b2(seedInput);
        int resIndex = resolutionPicker->findData(project->textureWidth);
        resolutionPicker->setCurrentIndex(resIndex >= 0 ? resIndex : 5);
        resolutionPicker->setEnabled(true);
        seedInput->setValue(project->randomSeed);
        seedInput->setEnabled(true);
    }

    // Set library for search popup
    if (project && project->library) {
        searchPopup->setLibrary(project->library);
    }

    // add nodes
    for (auto node : project->nodes) {
        this->addNode(node);
    }

    // add connections
    for (auto con : project->connections) {
        auto leftNode = scene->getNodeById(con->leftNode->id);
        auto rightNode = scene->getNodeById(con->rightNode->id);

        scene->connectNodes(leftNode, "output", rightNode,
                            con->rightNodeInputName);
    }

    // add comments
    for (auto comment : project->comments) {
        auto gcomment = nodegraph::Comment::create();
        gcomment->setId(comment->id);
        gcomment->setText(comment->text);
        gcomment->setPos(comment->pos.x(), comment->pos.y());
        scene->addComment(gcomment);
    }

    // add frames
    for (auto frame : project->frames) {
        auto gframe = nodegraph::Frame::create();
        gframe->setId(frame->id);
        gframe->setTitle(frame->text);
        gframe->setPos(frame->pos.x(), frame->pos.y());
        if (frame->size.x() > 0 && frame->size.y() > 0)
            gframe->setSize(frame->size.x(), frame->size.y());
        scene->addFrame(gframe);
    }

    // graph->setNodeGraphScene(nodegraph::ScenePtr(scene));
    graph->setNodeGraphScene(scene);
}

void GraphWidget::addNode(const TextureNodePtr& node)
{
    auto gnode = nodegraph::Node::create();
    gnode->setName(node->title);
    for (auto input : node->inputs) {
        gnode->addInPort(input);
    }

    gnode->setId(node->id);
    gnode->addOutPort("output");
    gnode->setCenter(node->pos.x(), node->pos.y());

    scene->addNode(gnode);
}

void GraphWidget::syncPositionsToModel()
{
    if (!project || !scene)
        return;

    for (auto& node : project->nodes) {
        auto gnode = scene->getNodeById(node->id);
        if (gnode) {
            auto center = gnode->getCenter();
            node->pos = QVector2D(center.x(), center.y());
        }
    }

    for (auto& comment : project->comments) {
        auto gcomment = scene->getCommentById(comment->id);
        if (gcomment) {
            auto p = gcomment->pos();
            comment->pos = QVector2D(p.x(), p.y());
        }
    }

    for (auto& frame : project->frames) {
        auto gframe = scene->getFrameById(frame->id);
        if (gframe) {
            auto p = gframe->pos();
            frame->pos = QVector2D(p.x(), p.y());
            auto rect = gframe->frameRect();
            frame->size = QVector2D(rect.width(), rect.height());
        }
    }
}

void GraphWidget::dragEnterEvent(QDragEnterEvent* evt)
{
    // qDebug() << "Drag enter";
    evt->acceptProposedAction();
}

void GraphWidget::dragMoveEvent(QDragMoveEvent* evt)
{
    // qDebug() << "drag move";
    evt->acceptProposedAction();
}

void GraphWidget::dropEvent(QDropEvent* evt)
{
    auto mimeData = evt->mimeData();
    if (mimeData->hasFormat(LIBRARY_ITEM_MIME_FORMAT)) {
        auto data = (const LibraryItemMimeData*)mimeData;
        auto scenePos = this->graph->mapToScene(evt->position().toPoint());

        if (data->itemType == PopupItemType::Frame) {
            auto frame = nodegraph::Frame::create();
            frame->setPos(scenePos);
            scene->addFrame(frame);
        }
        else if (data->itemType == PopupItemType::Comment) {
            auto comment = nodegraph::Comment::create();
            comment->setPos(scenePos);
            scene->addComment(comment);
        }
        else {
            auto node = project->library->createNode(data->libraryItemName);
            node->pos = QVector2D(scenePos);
            this->project->addNode(node);
            this->addNode(node);
            this->renderer->update();
        }

        evt->accept();
    }
}

void GraphWidget::setTextureRenderer(TextureRenderer* renderer)
{
    this->renderer = renderer;

    connect(renderer, &TextureRenderer::thumbnailGenerated,
            [=](const QString& nodeId, GLint texId, const QPixmap& pixmap) {
                // scene->setNodeThumbnail(nodeId, pixmap);
                auto node = scene->getNodeById(nodeId);
                if (node) {
                    node->setTextureId(texId);
                    node->setThumbnail(pixmap);
                }
            });
}

void GraphWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space) {
        // Show the search popup at the current mouse cursor position
        QPoint globalPos = QCursor::pos();
        searchPopup->show(globalPos);
        event->accept();
    }
    else {
        QMainWindow::keyPressEvent(event);
    }
}

void GraphWidget::addItemFromSearch(const QString& name, PopupItemType type,
                                    const QPoint& position)
{
    QPoint localPos = graph->mapFromGlobal(position);
    auto scenePos = graph->mapToScene(localPos);

    if (type == PopupItemType::Frame) {
        auto frame = nodegraph::Frame::create();
        frame->setPos(scenePos);
        scene->addFrame(frame);
    }
    else if (type == PopupItemType::Comment) {
        auto comment = nodegraph::Comment::create();
        comment->setPos(scenePos);
        scene->addComment(comment);
    }
    else {
        if (!project || !project->library)
            return;

        auto node = project->library->createNode(name);
        node->pos = QVector2D(scenePos);
        this->project->addNode(node);
        this->addNode(node);

        if (this->renderer) {
            this->renderer->update();
        }
    }
}