#include "graphwidget.h"
#include "../clipboard.h"
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
#include <QShortcut>
#include <QSignalBlocker>
#include <QToolBar>

class NoWheelComboBox : public QComboBox {
public:
    using QComboBox::QComboBox;
    void wheelEvent(QWheelEvent* event) override { event->ignore(); }
};

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

void GraphWidget::syncFrameToScene(const FramePtr& frame)
{
    if (!frame || !scene)
        return;
    auto ngFrame = scene->getFrameById(frame->id);
    if (ngFrame) {
        ngFrame->setTitle(frame->text);
        ngFrame->setColor(frame->color);
    }
}

void GraphWidget::syncCommentToScene(const CommentPtr& comment)
{
    if (!comment || !scene)
        return;
    auto ngComment = scene->getCommentById(comment->id);
    if (ngComment)
        ngComment->setText(comment->text);
}

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

    connect(graph, &nodegraph::NodeGraph::nodeRemoved,
            [=](nodegraph::NodePtr node) {
                auto nodeId = node->id();
                auto texNode = project->getNodeById(nodeId);

                // remove all connections involving this node from the model
                for (auto key : project->connections.keys()) {
                    auto con = project->connections[key];
                    if (con->leftNode->id == nodeId ||
                        con->rightNode->id == nodeId) {
                        // mark downstream node dirty before disconnecting
                        if (con->leftNode->id == nodeId)
                            con->rightNode->isDirty = true;
                        project->connections.remove(key);
                    }
                }

                project->nodes.remove(nodeId);

                emit nodeSelectionChanged(TextureNodePtr(nullptr));
                renderer->update();
            });

    connect(graph, &nodegraph::NodeGraph::frameSelectionChanged,
            [=](nodegraph::FramePtr ngFrame) {
                if (!ngFrame || !project) {
                    emit frameSelectionChanged(FramePtr(nullptr));
                    return;
                }
                auto modelFrame = project->frames.value(ngFrame->id());
                emit frameSelectionChanged(modelFrame);
            });

    connect(graph, &nodegraph::NodeGraph::commentSelectionChanged,
            [=](nodegraph::CommentPtr ngComment) {
                if (!ngComment || !project) {
                    emit commentSelectionChanged(CommentPtr(nullptr));
                    return;
                }
                auto modelComment = project->comments.value(ngComment->id());
                emit commentSelectionChanged(modelComment);
            });

    // library = nullptr;

    auto copyShortcut = new QShortcut(QKeySequence::Copy, this);
    copyShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(copyShortcut, &QShortcut::activated, this, &GraphWidget::executeCopy);

    auto cutShortcut = new QShortcut(QKeySequence::Cut, this);
    cutShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(cutShortcut, &QShortcut::activated, this, &GraphWidget::executeCut);

    auto pasteShortcut = new QShortcut(QKeySequence::Paste, this);
    pasteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(pasteShortcut, &QShortcut::activated, this, &GraphWidget::executePaste);
}

void GraphWidget::setupToolbar()
{
    auto toolbar = this->addToolBar("Graph");
    toolbar->setMovable(false);

    toolbar->addWidget(new QLabel("Resolution: "));

    resolutionPicker = new NoWheelComboBox();
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
        gframe->setColor(frame->color);
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

            if (project) {
                auto modelFrame = FramePtr(new Frame());
                modelFrame->id = frame->id();
                modelFrame->text = frame->title();
                modelFrame->pos = QVector2D(scenePos.x(), scenePos.y());
                project->frames[modelFrame->id] = modelFrame;
            }
        }
        else if (data->itemType == PopupItemType::Comment) {
            auto comment = nodegraph::Comment::create();
            comment->setPos(scenePos);
            scene->addComment(comment);

            if (project) {
                auto modelComment = CommentPtr(new Comment());
                modelComment->id = comment->id();
                modelComment->text = comment->text();
                modelComment->pos = QVector2D(scenePos.x(), scenePos.y());
                project->comments[modelComment->id] = modelComment;
            }
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

void GraphWidget::executeCopy()
{
    if (!project || !scene)
        return;

    syncPositionsToModel();

    QList<QString> nodeIds, frameIds, commentIds;
    for (auto item : scene->selectedItems()) {
        if (item->type() == (int)nodegraph::SceneItemType::Node) {
            auto node = qgraphicsitem_cast<nodegraph::Node*>(item);
            if (node)
                nodeIds.append(node->id());
        }
        else if (item->type() == (int)nodegraph::SceneItemType::Frame) {
            auto frame = qgraphicsitem_cast<nodegraph::Frame*>(item);
            if (frame)
                frameIds.append(frame->id());
        }
        else if (item->type() == (int)nodegraph::SceneItemType::Comment) {
            auto comment = qgraphicsitem_cast<nodegraph::Comment*>(item);
            if (comment)
                commentIds.append(comment->id());
        }
    }

    if (nodeIds.isEmpty() && frameIds.isEmpty() && commentIds.isEmpty())
        return;

    Clipboard::copyItems(project, nodeIds, frameIds, commentIds);
}

void GraphWidget::executeCut()
{
    if (!project || !scene)
        return;

    executeCopy();

    // Collect IDs before modifying the scene
    QList<QString> nodeIds, frameIds, commentIds;
    for (auto item : scene->selectedItems()) {
        if (item->type() == (int)nodegraph::SceneItemType::Node) {
            auto node = qgraphicsitem_cast<nodegraph::Node*>(item);
            if (node)
                nodeIds.append(node->id());
        }
        else if (item->type() == (int)nodegraph::SceneItemType::Frame) {
            auto frame = qgraphicsitem_cast<nodegraph::Frame*>(item);
            if (frame)
                frameIds.append(frame->id());
        }
        else if (item->type() == (int)nodegraph::SceneItemType::Comment) {
            auto comment = qgraphicsitem_cast<nodegraph::Comment*>(item);
            if (comment)
                commentIds.append(comment->id());
        }
    }

    // Remove nodes, propagating dirty through the full downstream subgraph
    for (const auto& id : nodeIds) {
        auto ngNode = scene->getNodeById(id);
        if (ngNode)
            scene->removeNode(ngNode);

        // Capture downstream nodes before their connections are removed
        auto downstream = project->getNodeRightOfNode(id);

        for (auto key : project->connections.keys()) {
            auto con = project->connections[key];
            if (con->leftNode->id == id || con->rightNode->id == id)
                project->connections.remove(key);
        }

        // BFS-mark all transitive dependents dirty so they re-render
        for (auto& dep : downstream)
            project->markNodeAsDirty(dep);

        project->nodes.remove(id);
    }

    // Remove frames
    for (const auto& id : frameIds) {
        auto ngFrame = scene->getFrameById(id);
        if (ngFrame)
            scene->removeFrame(ngFrame);
        project->frames.remove(id);
    }

    // Remove comments
    for (const auto& id : commentIds) {
        auto ngComment = scene->getCommentById(id);
        if (ngComment)
            scene->removeComment(ngComment);
        project->comments.remove(id);
    }

    // Clear properties panel regardless of which item type was selected
    emit nodeSelectionChanged(TextureNodePtr(nullptr));
    emit frameSelectionChanged(FramePtr(nullptr));
    emit commentSelectionChanged(CommentPtr(nullptr));

    scene->update();
    if (renderer)
        renderer->update();
}

void GraphWidget::executePaste()
{
    if (!project || !scene)
        return;

    QList<TextureNodePtr> newNodes;
    QList<ConnectionPtr> newConnections;
    QList<CommentPtr> newComments;
    QList<FramePtr> newFrames;

    QPointF viewCenter = graph->mapToScene(graph->viewport()->rect().center());

    if (!Clipboard::pasteItems(project, viewCenter, newNodes, newConnections,
                               newComments, newFrames))
        return;

    scene->clearSelection();

    // Add nodes
    for (auto& node : newNodes) {
        project->nodes[node->id] = node;
        addNode(node);
        auto ngNode = scene->getNodeById(node->id);
        if (ngNode)
            ngNode->setSelected(true);
    }

    // Add connections and invalidate the receiving node so it re-renders
    for (auto& con : newConnections) {
        project->connections[con->id] = con;
        con->rightNode->isDirty = true;
        auto leftNgNode = scene->getNodeById(con->leftNode->id);
        auto rightNgNode = scene->getNodeById(con->rightNode->id);
        if (leftNgNode && rightNgNode)
            scene->connectNodes(leftNgNode, "output", rightNgNode,
                                con->rightNodeInputName);
    }

    // Add comments
    for (auto& comment : newComments) {
        project->comments[comment->id] = comment;
        auto gcomment = nodegraph::Comment::create();
        gcomment->setId(comment->id);
        gcomment->setText(comment->text);
        gcomment->setPos(comment->pos.x(), comment->pos.y());
        scene->addComment(gcomment);
        gcomment->setSelected(true);
    }

    // Add frames
    for (auto& frame : newFrames) {
        project->frames[frame->id] = frame;
        auto gframe = nodegraph::Frame::create();
        gframe->setId(frame->id);
        gframe->setTitle(frame->text);
        gframe->setColor(frame->color);
        gframe->setPos(frame->pos.x(), frame->pos.y());
        if (frame->size.x() > 0 && frame->size.y() > 0)
            gframe->setSize(frame->size.x(), frame->size.y());
        scene->addFrame(gframe);
        gframe->setSelected(true);
    }

    scene->update();
    if (renderer)
        renderer->update();
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

        if (project) {
            auto modelFrame = FramePtr(new Frame());
            modelFrame->id = frame->id();
            modelFrame->text = frame->title();
            modelFrame->pos = QVector2D(scenePos.x(), scenePos.y());
            project->frames[modelFrame->id] = modelFrame;
        }
    }
    else if (type == PopupItemType::Comment) {
        auto comment = nodegraph::Comment::create();
        comment->setPos(scenePos);
        scene->addComment(comment);

        if (project) {
            auto modelComment = CommentPtr(new Comment());
            modelComment->id = comment->id();
            modelComment->text = comment->text();
            modelComment->pos = QVector2D(scenePos.x(), scenePos.y());
            project->comments[modelComment->id] = modelComment;
        }
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