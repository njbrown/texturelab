#include <QtWidgets/QGraphicsScene>

#include <QtGui/QPen>
#include <QtGui/QBrush>
#include <QtWidgets/QMenu>

#include <QtCore/QRectF>
#include <QtCore/QPointF>
// #include <QtCore/QColor>

// #include <QtOpenGL>
#include <QtWidgets>

#include <QDebug>
#include <iostream>
#include <cmath>

const QColor BackgroundColor(53, 53, 53);
const QColor FineGridColor(60, 60, 60);
const QColor CoarseGridColor(25, 25, 25);

#include "nodegraph.h"
#include "graph/scene.h"

MouseButtonStates::MouseButtonStates()
{
    reset();
}

void MouseButtonStates::reset()
{
    left = false;
    middle = false;
    right = false;
}

NodeGraph::
    NodeGraph(QWidget *parent)
    : QGraphicsView(parent)
{
    setDragMode(QGraphicsView::RubberBandDrag);
    setRenderHint(QPainter::Antialiasing);

    // setBackgroundBrush(BackgroundColor);
    setBackgroundBrush(QColor(53, 53, 53));

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    setCacheMode(QGraphicsView::CacheBackground);
    // setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));

    // auto scene = new QGraphicsScene();
    // scene->addText("Hello World!");
    // setScene(scene);

    this->setNodeGraphScene(ScenePtr(new Scene()));
    mbStates.reset();
}

void NodeGraph::setNodeGraphScene(ScenePtr scene)
{
    this->_scene = scene;
    this->setScene(scene.data());

    // handle scene's events from within the view
    scene->installEventFilter(this);
}

void NodeGraph::
    wheelEvent(QWheelEvent *event)
{
    QPoint delta = event->angleDelta();

    if (delta.y() == 0)
    {
        event->ignore();
        return;
    }

    double const d = delta.y() / std::abs(delta.y());

    if (d > 0.0)
        scaleUp();
    else
        scaleDown();
}

void NodeGraph::
    scaleUp()
{
    double const step = 1.2;
    double const factor = std::pow(step, 1.0);

    QTransform t = transform();

    if (t.m11() > 2.0)
        return;

    scale(factor, factor);
}

void NodeGraph::
    scaleDown()
{
    double const step = 1.2;
    double const factor = std::pow(step, -1.0);

    scale(factor, factor);
}

void NodeGraph::
    keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete)
    {
        // delete items!
        auto items = this->_scene->selectedItems();
        for (auto item : items)
        {
            if (item->type() == (int)SceneItemType::Node)
            {
                auto node = qgraphicsitem_cast<Node *>(item);
                this->_scene->removeNode(node->sharedFromThis());
            }
        }

        // package them and pass them to deletion signal
    }

    QGraphicsView::keyPressEvent(event);

    this->invalidateScene(QRect(-1000, -1000, 1000, 1000));
}

void NodeGraph::
    keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Shift:
        // setDragMode(QGraphicsView::ScrollHandDrag);
        break;

    default:
        break;
    }
    QGraphicsView::keyReleaseEvent(event);
}

void NodeGraph::
    mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && scene()->mouseGrabberItem() == nullptr)
    {
        _clickPos = mapToScene(event->pos());
        setDragMode(QGraphicsView::NoDrag);
    }
    QGraphicsView::mousePressEvent(event);
}

void NodeGraph::
    mouseMoveEvent(QMouseEvent *event)
{

    if (event->buttons() == Qt::MiddleButton)
    {
        QPointF difference = _clickPos - mapToScene(event->pos());
        setSceneRect(sceneRect().translated(difference.x(), difference.y()));
    }
    QGraphicsView::mouseMoveEvent(event);
}

void NodeGraph::
    mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        setDragMode(QGraphicsView::RubberBandDrag);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void NodeGraph::
    drawBackground(QPainter *painter, const QRectF &r)
{
    QGraphicsView::drawBackground(painter, r);
    painter->setRenderHint(QPainter::Antialiasing);

    auto drawGrid =
        [&](double gridStep)
    {
        QRect windowRect = rect();
        QPointF tl = mapToScene(windowRect.topLeft());
        QPointF br = mapToScene(windowRect.bottomRight());

        double left = std::floor(tl.x() / gridStep - 0.5);
        double right = std::floor(br.x() / gridStep + 1.0);
        double bottom = std::floor(tl.y() / gridStep - 0.5);
        double top = std::floor(br.y() / gridStep + 1.0);

        // vertical lines
        for (int xi = int(left); xi <= int(right); ++xi)
        {
            QLineF line(xi * gridStep, bottom * gridStep,
                        xi * gridStep, top * gridStep);

            painter->drawLine(line);
        }

        // horizontal lines
        for (int yi = int(bottom); yi <= int(top); ++yi)
        {
            QLineF line(left * gridStep, yi * gridStep,
                        right * gridStep, yi * gridStep);
            painter->drawLine(line);
        }
    };

    QBrush bBrush = backgroundBrush();

    QPen pfine(FineGridColor, 1.0);

    painter->setPen(pfine);
    drawGrid(15);

    QPen p(CoarseGridColor, 1.0);

    painter->setPen(p);
    drawGrid(150);
}

void NodeGraph::
    showEvent(QShowEvent *event)
{
    // _scene->setSceneRect(this->rect());
    QGraphicsView::showEvent(event);
}

bool NodeGraph::eventFilter(QObject *o, QEvent *e)
{
    QGraphicsSceneMouseEvent *me = (QGraphicsSceneMouseEvent *)e;
    if (o == _scene.data())
    {

        switch ((int)e->type())
        {
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
bool NodeGraph::sceneMousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // qDebug() << "Mouse Press!";
    if (event->button() == Qt::LeftButton)
        mbStates.left = true;
    if (event->button() == Qt::MiddleButton)
        mbStates.middle = true;
    if (event->button() == Qt::RightButton)
        mbStates.right = true;

    // check for hit socket if left button is pressed
    if (mbStates.left)
    {
        auto scenePos = event->scenePos();
        auto rawPort = this->getPortAtScenePos(scenePos.x(), scenePos.y());
        if (rawPort)
        {
            // auto port = rawPort->node->getPortById(rawPort->id());
            // gotta cast to get the non-const version
            PortPtr port = ((Port *)rawPort)->sharedFromThis();

            // are we modifying an existing port with a connection?
            if (port->portType == PortType::In)
            {
                // in-sockets with an active connection are the only
                // ones that can be edited, and that's the case here
                if (port->connections.count() > 0)
                {
                    // get the connection
                    auto con = port->connections[0];

                    // remove it
                    _scene->removeConnection(con);

                    // make it activeCon
                    con->endPort.clear();
                    con->pos1 = con->startPort->scenePos();
                    con->pos2 = scenePos;
                    activeCon = con;
                    activeCon->updatePathFromPositions();
                    activeCon->connectState = ConnectionState::Dragging;

                    // emit connection removal signal

                    this->_scene->addItem(activeCon.data());
                    setDragMode(QGraphicsView::NoDrag);
                    return true;
                }
                else
                {
                    // allow starting connection from left to right?
                }
            }
            // start new connection
            else if (port->portType != PortType::Invalid /* in or out */)
            {
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
            else
            {
                qDebug() << "trying to hit an Invalid socket";
            }

            this->setDragMode(QGraphicsView::NoDrag);
        }
    }

    return false;
}

bool NodeGraph::sceneMouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    auto scenePos = event->scenePos();
    if (mbStates.left && !!activeCon)
    {
        auto rawPort = this->getPortAtScenePos(scenePos.x(), scenePos.y());
        if (rawPort)
        {
            // snap if close enough
            activeCon->pos2 = rawPort->scenePos();
        }
        else
        {
            activeCon->pos2 = scenePos;
        }
        activeCon->updatePathFromPositions();
    }

    return false;
}

bool NodeGraph::sceneMouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        mbStates.left = false;
    if (event->button() == Qt::MiddleButton)
        mbStates.middle = false;
    if (event->button() == Qt::RightButton)
        mbStates.right = false;

    auto scenePos = event->scenePos();

    if (mbStates.left == false && !!activeCon)
    {
        auto rawPort = this->getPortAtScenePos(scenePos.x(), scenePos.y());
        if (rawPort)
        {
            // auto hitPort = rawPort->node->getPortById(rawPort->id());
            // gotta cast to get the non-const version
            PortPtr hitPort = ((Port *)rawPort)->sharedFromThis();

            // determine between the in and out sockets
            PortPtr leftPort;
            PortPtr rightPort;

            if (hitPort->portType == PortType::In)
            {
                leftPort = activeCon->startPort;
                rightPort = hitPort;
            }
            else
            {
                leftPort = hitPort;
                rightPort = activeCon->startPort;
            }

            // check validity of the potential connection
            // NOTE: there's still a chance both leftPort and rightPort are
            // still the same type or the same port

            bool isConValid = true;

            if (leftPort == rightPort)
            {
                isConValid = false;
                qDebug() << "invalid port";
            }

            if (leftPort->portType == rightPort->portType)
            {
                isConValid = false;
                qDebug() << "ports are the same";
            }

            // it's okay to check this since the two prior checks
            // would have invalidated same-type and same-port connections
            // todo: override connection
            if (rightPort->connections.count() != 0)
            {
                isConValid = false;
                qDebug() << "right port has existing connections";
            }

            // todo: check for cycle

            if (isConValid)
            {
                // actually make connection
                _scene->connectNodes(leftPort->node, leftPort->name, rightPort->node, rightPort->name);

                // todo: emit undo task
            }
        }

        // remove from scene
        _scene->removeItem(activeCon.data());
        activeCon.clear();
    }

    // important to reset drag!
    this->setDragMode(QGraphicsView::RubberBandDrag);
    return false;
}

const Port *NodeGraph::getPortAtScenePos(float x, float y) const
{
    auto items = this->_scene->items(QPointF(x, y));
    // auto items = this->items();
    for (auto item : items)
    {
        if (item && item->type() == (int)SceneItemType::Port)
            return (const Port *)item;
    }

    return nullptr;
}

NodeGraph::~NodeGraph()
{
    // remove all items manually otherwise
    // smart point destructor of some items
    // will cause segfault when cleaning up
    // todo: move this to scene items
    auto items = this->_scene->items();
    for (auto item : items)
        this->_scene->removeItem(item);
}