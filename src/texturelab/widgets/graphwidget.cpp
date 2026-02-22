#include "graphwidget.h"
#include <QCursor>
#include <QDragEnterEvent>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QMouseEvent>
#include <QOpenGLContext>
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

void GraphWidget::setTextureProject(TextureProjectPtr project)
{
    // generate nodes from texture project

    //   auto scene = graph->scene();
    auto scene = nodegraph::Scene::create();
    // auto scene = new nodegraph::Scene();
    this->scene = scene;
    this->project = project;

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

    // todo: add frames
    // todo: add comments
    // todo: add navigations

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
    gnode->setPos(node->pos.x(), node->pos.y());

    scene->addNode(gnode);
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
            node->pos = QVector2D(scenePos) - QVector2D(50, 50);
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
        node->pos = QVector2D(scenePos) - QVector2D(50, 50);
        this->project->addNode(node);
        this->addNode(node);

        if (this->renderer) {
            this->renderer->update();
        }
    }
}