#include <QtWidgets/QGraphicsScene>

#include <QtGui/QBrush>
#include <QtGui/QPen>
#include <QtWidgets/QMenu>

#include <QtCore/QPointF>
#include <QtCore/QRectF>
// #include <QtCore/QColor>

// #include <QtOpenGL>
#include <QOpenGLWidget>
#include <QtWidgets>

#include <QDebug>
#include <cmath>
#include <iostream>

#include "graph/comment.h"
#include "graph/frame.h"
#include "graph/nodetheme.h"
#include "graph/scene.h"
#include "nodegraph.h"

namespace nodegraph {

MouseButtonStates::MouseButtonStates() { reset(); }

void MouseButtonStates::reset()
{
    left = false;
    middle = false;
    right = false;
}

NodeGraph::NodeGraph(QWidget* parent) : QGraphicsView(parent)
{
    // https://doc.qt.io/qt-6.2/graphicsview.html#opengl-rendering
    auto gl = new QOpenGLWidget();
    QSurfaceFormat format;
    format.setSamples(4);
    gl->setFormat(format);
    this->setViewport(gl);

    setDragMode(QGraphicsView::RubberBandDrag);
    setRenderHint(QPainter::Antialiasing);

    setBackgroundBrush(ntColor(Tokens::GridBg));

    // Repaint (and refresh the themed background brush) whenever the theme
    // changes, so the node graph follows --dev-theme hot-reloads like the rest
    // of the app. drawBackground() reads grid colors from tokens at paint time.
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
                     [this]() {
                         setBackgroundBrush(ntColor(Tokens::GridBg));
                         if (scene())
                             scene()->update();
                         viewport()->update();
                     });

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // setCacheMode(QGraphicsView::CacheBackground);
    // setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));

    // auto scene = new QGraphicsScene();
    // scene->addText("Hello World!");
    // setScene(scene);

    setAcceptDrops(true);

    this->setNodeGraphScene(ScenePtr(new Scene()));
    mbStates.reset();
}

void NodeGraph::setNodeGraphScene(const ScenePtr& scene)
{
    // properly cleanup old scene
    if (!!this->_scene) {
        this->setScene(nullptr);
    }

    this->_scene = scene;
    scene->setSceneRect(-100000, -100000, 200000, 200000);
    this->setScene(scene.data());

    // handle scene's events from within the view
    scene->installEventFilter(this);

    // connect to selection signal
    connect(scene.data(), &QGraphicsScene::selectionChanged,
            [=]() { this->handleSelectionChange(); });
}

void NodeGraph::wheelEvent(QWheelEvent* event)
{
    QPoint delta = event->angleDelta();

    if (delta.y() == 0) {
        event->ignore();
        return;
    }

    double const d = delta.y() / std::abs(delta.y());

    if (d > 0.0)
        scaleUp();
    else
        scaleDown();
}

void NodeGraph::scaleUp()
{
    double const step = 1.2;
    double const factor = std::pow(step, 1.0);

    QTransform t = transform();

    if (t.m11() > 2.0)
        return;

    scale(factor, factor);
}

void NodeGraph::scaleDown()
{
    double const step = 1.2;
    double const factor = std::pow(step, -1.0);

    scale(factor, factor);
}

void NodeGraph::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete) {
        QList<NodePtr> selectedNodes;
        QList<FramePtr> selectedFrames;
        QList<CommentPtr> selectedComments;

        for (auto item : this->_scene->selectedItems()) {
            if (item->type() == (int)SceneItemType::Node)
                selectedNodes.append(qgraphicsitem_cast<Node*>(item)->sharedFromThis());
            else if (item->type() == (int)SceneItemType::Frame)
                selectedFrames.append(qgraphicsitem_cast<Frame*>(item)->sharedFromThis());
            else if (item->type() == (int)SceneItemType::Comment)
                selectedComments.append(qgraphicsitem_cast<Comment*>(item)->sharedFromThis());
        }

        if (!selectedNodes.isEmpty() || !selectedFrames.isEmpty() || !selectedComments.isEmpty())
            emit deleteRequested(selectedNodes, selectedFrames, selectedComments);
    }

    QGraphicsView::keyPressEvent(event);
    this->invalidateScene(QRect(-1000, -1000, 1000, 1000));
}

void NodeGraph::keyReleaseEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Shift:
        // setDragMode(QGraphicsView::ScrollHandDrag);
        break;

    default:
        break;
    }
    QGraphicsView::keyReleaseEvent(event);
}

void NodeGraph::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        _clickPos = event->pos();
        setDragMode(QGraphicsView::NoDrag);
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void NodeGraph::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::MiddleButton) {
        QPointF delta = event->pos() - _clickPos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        _clickPos = event->pos();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void NodeGraph::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        setDragMode(QGraphicsView::RubberBandDrag);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void NodeGraph::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {

        auto scenePos = this->mapToScene(event->pos());

        // node: stripped const from returned node
        auto node = (Node*)getNodeAtScenePos(scenePos.x(), scenePos.y());
        if (node) {
            emit nodeDoubleClicked(node->sharedFromThis());
        }
        else {
            emit nodeDoubleClicked(nullptr);
        }
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void NodeGraph::drawBackground(QPainter* painter, const QRectF& r)
{
    auto type = painter->paintEngine()->type();
    if (type != QPaintEngine::OpenGL && type != QPaintEngine::OpenGL2) {
        qWarning() << "background paint engine needs to be OPENGL!";
        // return;
    }

    QGraphicsView::drawBackground(painter, r);
    painter->setRenderHint(QPainter::Antialiasing);

    auto drawGrid = [&](double gridStep) {
        QRect windowRect = rect();
        QPointF tl = mapToScene(windowRect.topLeft());
        QPointF br = mapToScene(windowRect.bottomRight());

        double left = std::floor(tl.x() / gridStep - 0.5);
        double right = std::floor(br.x() / gridStep + 1.0);
        double bottom = std::floor(tl.y() / gridStep - 0.5);
        double top = std::floor(br.y() / gridStep + 1.0);

        // vertical lines
        for (int xi = int(left); xi <= int(right); ++xi) {
            QLineF line(xi * gridStep, bottom * gridStep, xi * gridStep,
                        top * gridStep);

            painter->drawLine(line);
        }

        // horizontal lines
        for (int yi = int(bottom); yi <= int(top); ++yi) {
            QLineF line(left * gridStep, yi * gridStep, right * gridStep,
                        yi * gridStep);
            painter->drawLine(line);
        }
    };

    QPen pfine(ntColor(Tokens::GridFine), 1.0);

    painter->setPen(pfine);
    drawGrid(15);

    QPen p(ntColor(Tokens::GridCoarse), 1.0);

    painter->setPen(p);
    drawGrid(150);
}

void NodeGraph::showEvent(QShowEvent* event)
{
    // _scene->setSceneRect(this->rect());
    QGraphicsView::showEvent(event);
}

bool NodeGraph::eventFilter(QObject* o, QEvent* e)
{
    QGraphicsSceneMouseEvent* me = (QGraphicsSceneMouseEvent*)e;
    if (o == _scene.data()) {

        switch ((int)e->type()) {
        case QEvent::GraphicsSceneMousePress:
            if (this->sceneMousePressEvent(me))
                return true;
            break;
        case QEvent::GraphicsSceneMouseMove:
            if (this->sceneMouseMoveEvent(me))
                return true;
            break;
        case QEvent::GraphicsSceneMouseRelease:
            if (this->sceneMouseReleaseEvent(me))
                return true;
            break;

            // case QEvent::GraphicsSceneDrop:
            //     break;
        }
    }

    return QObject::eventFilter(o, e);
}

// todo: probably best to handle this in the views mousePressEvent
bool NodeGraph::sceneMousePressEvent(QGraphicsSceneMouseEvent* event)
{
    // qDebug() << "Mouse Press!";
    if (event->button() == Qt::LeftButton)
        mbStates.left = true;
    if (event->button() == Qt::MiddleButton)
        mbStates.middle = true;
    if (event->button() == Qt::RightButton)
        mbStates.right = true;

    // check for hit socket if left button is pressed
    if (mbStates.left) {
        auto scenePos = event->scenePos();
        auto rawPort = this->getPortAtScenePos(scenePos.x(), scenePos.y());
        if (!rawPort) {
            // Record node positions for move-tracking (no port drag starting)
            _preDragPositions.clear();
            for (auto& node : _scene->nodes)
                _preDragPositions[node->id()] = node->getCenter();
            _trackingMove = true;
        }
        if (rawPort) {
            // auto port = rawPort->node->getPortById(rawPort->id());
            // gotta cast to get the non-const version
            PortPtr port = ((Port*)rawPort)->sharedFromThis();

            // are we modifying an existing port with a connection?
            if (port->portType == PortType::In) {
                // in-sockets with an active connection are the only
                // ones that can be edited, and that's the case here
                if (port->connections.count() > 0) {
                    // get the connection
                    auto con = port->connections[0];

                    // emit connection removal signal
                    emit connectionRemoved(con);

                    // remove it
                    _scene->removeConnection(con);

                    // make it activeCon
                    con->endPort.clear();
                    con->pos1 = con->startPort->scenePos();
                    con->pos2 = scenePos;
                    activeCon = con;
                    activeCon->updatePathFromPositions();
                    activeCon->connectState = ConnectionState::Dragging;

                    this->_scene->addItem(activeCon.data());
                    setDragMode(QGraphicsView::NoDrag);
                    return true;
                }
                else {
                    // allow starting connection from left to right?
                }
            }
            // start new connection
            else if (port->portType != PortType::Invalid /* in or out */) {
                activeCon = ConnectionPtr(new Connection());
                activeCon->startPort = port;
                activeCon->connectState = ConnectionState::Dragging;

                activeCon->pos1 = port->scenePos();
                activeCon->pos2 = scenePos;
                activeCon->updatePathFromPositions();

                this->_scene->addItem(activeCon.data());

                // prevent further clicking of other items?
                // event->ignore();
                setDragMode(QGraphicsView::NoDrag);
                return true;
            }
            else {
                qDebug() << "trying to hit an Invalid socket";
            }

            this->setDragMode(QGraphicsView::NoDrag);
        }
    }

    return false;
}

bool NodeGraph::sceneMouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    auto scenePos = event->scenePos();
    if (mbStates.left && !!activeCon) {
        auto rawPort = this->getPortAtScenePos(scenePos.x(), scenePos.y());
        if (rawPort) {
            // snap if close enough
            activeCon->pos2 = rawPort->scenePos();
        }
        else {
            activeCon->pos2 = scenePos;
        }
        activeCon->updatePathFromPositions();

        // show socket names on nodes within proximity
        for (auto node : _nodesWithSocketNamesShown)
            node->setShowSocketNames(false);
        _nodesWithSocketNamesShown.clear();

        for (auto& nodePtr : _scene->nodes) {
            auto node = nodePtr.data();
            QPointF center = node->getCenter();
            qreal dx = center.x() - scenePos.x();
            qreal dy = center.y() - scenePos.y();
            if (dx * dx + dy * dy < SOCKET_LABEL_RADIUS * SOCKET_LABEL_RADIUS) {
                node->setShowSocketNames(true);
                _nodesWithSocketNamesShown.append(node);
            }
        }
    }

    return false;
}

bool NodeGraph::sceneMouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        mbStates.left = false;
    if (event->button() == Qt::MiddleButton)
        mbStates.middle = false;
    if (event->button() == Qt::RightButton)
        mbStates.right = false;

    auto scenePos = event->scenePos();

    if (mbStates.left == false && !!activeCon) {
        auto rawPort = this->getPortAtScenePos(scenePos.x(), scenePos.y());
        if (rawPort) {
            // auto hitPort = rawPort->node->getPortById(rawPort->id());
            // gotta cast to get the non-const version
            PortPtr hitPort = ((Port*)rawPort)->sharedFromThis();

            // determine between the in and out sockets
            PortPtr leftPort;
            PortPtr rightPort;

            if (hitPort->portType == PortType::In) {
                leftPort = activeCon->startPort;
                rightPort = hitPort;
            }
            else {
                leftPort = hitPort;
                rightPort = activeCon->startPort;
            }

            // check validity of the potential connection
            // NOTE: there's still a chance both leftPort and rightPort are
            // still the same type or the same port

            bool isConValid = true;

            if (leftPort == rightPort) {
                isConValid = false;
                qDebug() << "invalid port";
            }

            if (leftPort->portType == rightPort->portType) {
                isConValid = false;
                qDebug() << "ports are the same";
            }

            // it's okay to check this since the two prior checks
            // would have invalidated same-type and same-port connections
            // todo: override connection
            if (rightPort->connections.count() != 0) {
                isConValid = false;
                qDebug() << "right port has existing connections";
            }

            // todo: check for cycle

            if (isConValid) {
                // actually make connection
                auto con =
                    _scene->connectNodes(leftPort->node, leftPort->name,
                                         rightPort->node, rightPort->name);

                emit connectionAdded(con);
                // todo: emit undo task
            }
        }

        // clear proximity socket labels
        for (auto node : _nodesWithSocketNamesShown)
            node->setShowSocketNames(false);
        _nodesWithSocketNamesShown.clear();

        // remove from scene
        _scene->removeItem(activeCon.data());
        activeCon.clear();
    }

    // Emit move command if nodes changed position
    if (_trackingMove && !activeCon) {
        QMap<QString, QPointF> newPositions;
        bool moved = false;
        for (auto it = _preDragPositions.begin(); it != _preDragPositions.end(); ++it) {
            auto node = _scene->nodes.value(it.key());
            if (!node)
                continue;
            QPointF newPos = node->getCenter();
            newPositions[it.key()] = newPos;
            if (newPos != it.value())
                moved = true;
        }
        if (moved)
            emit itemsMoveFinished(_preDragPositions, newPositions);
    }
    _trackingMove = false;

    // important to reset drag!
    this->setDragMode(QGraphicsView::RubberBandDrag);
    return false;
}

void NodeGraph::dragEnterEvent(QDragEnterEvent* evt) { evt->ignore(); }

void NodeGraph::dragMoveEvent(QDragMoveEvent* evt) { evt->ignore(); }

void NodeGraph::dropEvent(QDropEvent* evt) { evt->ignore(); }

const Port* NodeGraph::getPortAtScenePos(float x, float y) const
{
    auto items = this->_scene->items(QPointF(x, y));
    // auto items = this->items();
    for (auto item : items) {
        if (item && item->type() == (int)SceneItemType::Port)
            return (const Port*)item;
    }

    return nullptr;
}

// get top-most node
const Node* NodeGraph::getNodeAtScenePos(float x, float y) const
{
    auto items = this->_scene->items(QPointF(x, y));

    for (auto item : items) {
        if (item && item->type() == (int)SceneItemType::Node)
            return (const Node*)item;
    }

    return nullptr;
}

void NodeGraph::handleSelectionChange()
{
    auto selected = this->_scene->selectedItems();
    for (auto item : selected) {
        if (item->type() == (int)SceneItemType::Node) {
            // emit nulls first so downstream handlers clear before setting new selection
            emit frameSelectionChanged(FramePtr(nullptr));
            emit commentSelectionChanged(CommentPtr(nullptr));
            emit nodeSelectionChanged(((Node*)item)->sharedFromThis());
            return;
        }
        if (item->type() == (int)SceneItemType::Frame) {
            emit nodeSelectionChanged(NodePtr(nullptr));
            emit commentSelectionChanged(CommentPtr(nullptr));
            emit frameSelectionChanged(((Frame*)item)->sharedFromThis());
            return;
        }
        if (item->type() == (int)SceneItemType::Comment) {
            emit nodeSelectionChanged(NodePtr(nullptr));
            emit frameSelectionChanged(FramePtr(nullptr));
            emit commentSelectionChanged(((Comment*)item)->sharedFromThis());
            return;
        }
    }

    emit nodeSelectionChanged(NodePtr(nullptr));
    emit frameSelectionChanged(FramePtr(nullptr));
    emit commentSelectionChanged(CommentPtr(nullptr));
}

NodeGraph::~NodeGraph() {}

} // namespace nodegraph