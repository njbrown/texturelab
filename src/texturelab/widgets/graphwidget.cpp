#include "graphwidget.h"
#include "../clipboard.h"
#include "../undo/undocommands.h"
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
#include <QUuid>

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
                auto leftNodeId = con->startPort->node->id();
                auto leftOutput = con->startPort->name;
                auto rightNodeId = con->endPort->node->id();
                auto rightInput = con->endPort->name;
                if (undoStack)
                    undoStack->push(new AddConnectionCommand(
                        project, scene, renderer, leftNodeId, leftOutput,
                        rightNodeId, rightInput));
                else {
                    // addConnection() marks the right node and everything
                    // downstream of it dirty
                    project->addConnection(project->getNodeById(leftNodeId),
                                           project->getNodeById(rightNodeId),
                                           rightInput);
                    renderer->update();
                }
            });

    connect(graph, &nodegraph::NodeGraph::connectionRemoved,
            [=](nodegraph::ConnectionPtr con) {
                auto leftNodeId = con->startPort->node->id();
                auto leftOutput = con->startPort->name;
                auto rightNodeId = con->endPort->node->id();
                auto rightInput = con->endPort->name;
                if (undoStack)
                    undoStack->push(new RemoveConnectionCommand(
                        project, scene, renderer, leftNodeId, leftOutput,
                        rightNodeId, rightInput));
                else {
                    // removeConnection() marks the right node and everything
                    // downstream of it dirty
                    project->removeConnection(leftNodeId, rightNodeId,
                                              rightInput);
                    renderer->update();
                }
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

    connect(
        graph, &nodegraph::NodeGraph::deleteRequested,
        [=](QList<nodegraph::NodePtr> nodes, QList<nodegraph::FramePtr> frames,
            QList<nodegraph::CommentPtr> comments) {
            QList<QString> nodeIds, frameIds, commentIds;
            for (auto& n : nodes)
                nodeIds.append(n->id());
            for (auto& f : frames)
                frameIds.append(f->id());
            for (auto& c : comments)
                commentIds.append(c->id());
            if (undoStack)
                undoStack->push(new DeleteItemsCommand(
                    project, scene, renderer, nodeIds, frameIds, commentIds));
            else {
                // Fallback: direct deletion (no undo)
                for (auto& n : nodes) {
                    scene->removeNode(n);

                    // also drops the node's connections and channel
                    // assignment, marking downstream nodes dirty
                    project->removeNode(n->id());
                }
                for (auto& f : frames) {
                    scene->removeFrame(f);
                    project->frames.remove(f->id());
                }
                for (auto& c : comments) {
                    scene->removeComment(c);
                    project->comments.remove(c->id());
                }
                renderer->update();
            }
            emit nodeSelectionChanged(TextureNodePtr(nullptr));
            emit frameSelectionChanged(FramePtr(nullptr));
            emit commentSelectionChanged(CommentPtr(nullptr));
        });

    connect(graph, &nodegraph::NodeGraph::itemsMoveFinished,
            [=](QMap<QString, QPointF> oldPos, QMap<QString, QPointF> newPos) {
                if (undoStack)
                    undoStack->push(
                        new MoveItemsCommand(project, scene, oldPos, newPos));
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
    connect(copyShortcut, &QShortcut::activated, this,
            &GraphWidget::executeCopy);

    auto cutShortcut = new QShortcut(QKeySequence::Cut, this);
    cutShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(cutShortcut, &QShortcut::activated, this, &GraphWidget::executeCut);

    auto pasteShortcut = new QShortcut(QKeySequence::Paste, this);
    pasteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(pasteShortcut, &QShortcut::activated, this,
            &GraphWidget::executePaste);
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
    // Only claim drags we actually handle (nodes/frames/comments dragged
    // from the Library panel). Anything else — e.g. a .texture file
    // dragged in from the OS — must be left ignored so Qt forwards it up
    // to MainWindow's dragEnterEvent instead of it being swallowed here.
    if (evt->mimeData()->hasFormat(LIBRARY_ITEM_MIME_FORMAT))
        evt->acceptProposedAction();
    else
        evt->ignore();
}

void GraphWidget::dragMoveEvent(QDragMoveEvent* evt)
{
    if (evt->mimeData()->hasFormat(LIBRARY_ITEM_MIME_FORMAT))
        evt->acceptProposedAction();
    else
        evt->ignore();
}

void GraphWidget::dropEvent(QDropEvent* evt)
{
    auto mimeData = evt->mimeData();
    if (mimeData->hasFormat(LIBRARY_ITEM_MIME_FORMAT)) {
        auto data = (const LibraryItemMimeData*)mimeData;
        auto scenePos = this->graph->mapToScene(evt->position().toPoint());

        if (data->itemType == PopupItemType::Frame) {
            QString frameId =
                QUuid::createUuid().toString(QUuid::WithoutBraces);
            if (undoStack)
                undoStack->push(new AddFrameCommand(project, scene, frameId,
                                                    QVector2D(scenePos)));
            else {
                auto frame = nodegraph::Frame::create();
                frame->setPos(scenePos);
                scene->addFrame(frame);
                if (project) {
                    auto modelFrame = FramePtr(new Frame());
                    modelFrame->id = frame->id();
                    modelFrame->pos = QVector2D(scenePos);
                    project->frames[modelFrame->id] = modelFrame;
                }
            }
        }
        else if (data->itemType == PopupItemType::Comment) {
            QString commentId =
                QUuid::createUuid().toString(QUuid::WithoutBraces);
            if (undoStack)
                undoStack->push(new AddCommentCommand(project, scene, commentId,
                                                      QVector2D(scenePos)));
            else {
                auto comment = nodegraph::Comment::create();
                comment->setPos(scenePos);
                scene->addComment(comment);
                if (project) {
                    auto modelComment = CommentPtr(new Comment());
                    modelComment->id = comment->id();
                    modelComment->pos = QVector2D(scenePos);
                    project->comments[modelComment->id] = modelComment;
                }
            }
        }
        else {
            if (undoStack)
                undoStack->push(new AddNodeCommand(project, scene, renderer,
                                                   data->libraryItemName,
                                                   QVector2D(scenePos)));
            else {
                auto node = project->library->createNode(data->libraryItemName);
                node->pos = QVector2D(scenePos);
                project->addNode(node);
                addNode(node);
                renderer->update();
            }
        }

        evt->accept();
    }
    else {
        evt->ignore();
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

    if (undoStack)
        undoStack->push(new DeleteItemsCommand(project, scene, renderer,
                                               nodeIds, frameIds, commentIds));
    else {
        for (const auto& id : nodeIds) {
            auto ngNode = scene->getNodeById(id);
            if (ngNode)
                scene->removeNode(ngNode);
            // also drops the node's connections and channel assignment,
            // marking downstream nodes dirty
            project->removeNode(id);
        }
        for (const auto& id : frameIds) {
            auto f = scene->getFrameById(id);
            if (f)
                scene->removeFrame(f);
            project->frames.remove(id);
        }
        for (const auto& id : commentIds) {
            auto c = scene->getCommentById(id);
            if (c)
                scene->removeComment(c);
            project->comments.remove(id);
        }
        if (renderer)
            renderer->update();
    }

    emit nodeSelectionChanged(TextureNodePtr(nullptr));
    emit frameSelectionChanged(FramePtr(nullptr));
    emit commentSelectionChanged(CommentPtr(nullptr));
}

void GraphWidget::executePaste()
{
    if (!project || !scene)
        return;

    QPointF viewCenter = graph->mapToScene(graph->viewport()->rect().center());

    if (undoStack) {
        auto* cmd = new PasteCommand(project, scene, renderer, viewCenter);
        if (cmd->isEmpty()) {
            delete cmd;
            return;
        }
        undoStack->push(cmd);
    }
    else {
        QList<TextureNodePtr> newNodes;
        QList<ConnectionPtr> newConnections;
        QList<CommentPtr> newComments;
        QList<FramePtr> newFrames;

        if (!Clipboard::pasteItems(project, viewCenter, newNodes,
                                   newConnections, newComments, newFrames))
            return;

        scene->clearSelection();
        for (auto& node : newNodes) {
            project->nodes[node->id] = node;
            addNode(node);
            auto ngNode = scene->getNodeById(node->id);
            if (ngNode)
                ngNode->setSelected(true);
        }
        for (auto& con : newConnections) {
            project->connections[con->id] = con;
            project->markNodeAsDirty(con->rightNode);
            auto l = scene->getNodeById(con->leftNode->id);
            auto r = scene->getNodeById(con->rightNode->id);
            if (l && r)
                scene->connectNodes(l, "output", r, con->rightNodeInputName);
        }
        for (auto& comment : newComments) {
            project->comments[comment->id] = comment;
            auto gc = nodegraph::Comment::create();
            gc->setId(comment->id);
            gc->setText(comment->text);
            gc->setPos(comment->pos.x(), comment->pos.y());
            scene->addComment(gc);
            gc->setSelected(true);
        }
        for (auto& frame : newFrames) {
            project->frames[frame->id] = frame;
            auto gf = nodegraph::Frame::create();
            gf->setId(frame->id);
            gf->setTitle(frame->text);
            gf->setColor(frame->color);
            gf->setPos(frame->pos.x(), frame->pos.y());
            if (frame->size.x() > 0 && frame->size.y() > 0)
                gf->setSize(frame->size.x(), frame->size.y());
            scene->addFrame(gf);
            gf->setSelected(true);
        }
        if (renderer)
            renderer->update();
    }
}

void GraphWidget::setUndoStack(QUndoStack* stack) { undoStack = stack; }

void GraphWidget::addItemFromSearch(const QString& name, PopupItemType type,
                                    const QPoint& position)
{
    QPoint localPos = graph->mapFromGlobal(position);
    auto scenePos = graph->mapToScene(localPos);

    if (type == PopupItemType::Frame) {
        QString frameId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (undoStack)
            undoStack->push(new AddFrameCommand(project, scene, frameId,
                                                QVector2D(scenePos)));
        else {
            auto frame = nodegraph::Frame::create();
            frame->setPos(scenePos);
            scene->addFrame(frame);
            if (project) {
                auto modelFrame = FramePtr(new Frame());
                modelFrame->id = frame->id();
                modelFrame->pos = QVector2D(scenePos);
                project->frames[modelFrame->id] = modelFrame;
            }
        }
    }
    else if (type == PopupItemType::Comment) {
        QString commentId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (undoStack)
            undoStack->push(new AddCommentCommand(project, scene, commentId,
                                                  QVector2D(scenePos)));
        else {
            auto comment = nodegraph::Comment::create();
            comment->setPos(scenePos);
            scene->addComment(comment);
            if (project) {
                auto modelComment = CommentPtr(new Comment());
                modelComment->id = comment->id();
                modelComment->pos = QVector2D(scenePos);
                project->comments[modelComment->id] = modelComment;
            }
        }
    }
    else {
        if (!project || !project->library)
            return;

        if (undoStack)
            undoStack->push(new AddNodeCommand(project, scene, renderer, name,
                                               QVector2D(scenePos)));
        else {
            auto node = project->library->createNode(name);
            node->pos = QVector2D(scenePos);
            project->addNode(node);
            addNode(node);
            if (renderer)
                renderer->update();
        }
    }
}