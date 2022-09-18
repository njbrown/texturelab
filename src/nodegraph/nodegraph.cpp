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

NodeGraph::
    NodeGraph(QWidget *parent)
    : QGraphicsView(parent)
{
    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHint(QPainter::Antialiasing);

    // setBackgroundBrush(BackgroundColor);
    setBackgroundBrush(QColor(53, 53, 53));

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    setCacheMode(QGraphicsView::CacheBackground);
    // setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // not needed rn
    // setDragMode(QGraphicsView::ScrollHandDrag);

    // setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));

    auto scene = new QGraphicsScene();
    scene->addText("Hello World!");
    setScene(scene);
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
    switch (event->key())
    {
    case Qt::Key_Shift:
        setDragMode(QGraphicsView::RubberBandDrag);
        break;

    default:
        break;
    }

    QGraphicsView::keyPressEvent(event);
}

void NodeGraph::
    keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Shift:
        setDragMode(QGraphicsView::ScrollHandDrag);
        break;

    default:
        break;
    }
    QGraphicsView::keyReleaseEvent(event);
}

void NodeGraph::
    mousePressEvent(QMouseEvent *event)
{

    if (event->button() == Qt::LeftButton)
    {
        _clickPos = mapToScene(event->pos());
    }
    QGraphicsView::mousePressEvent(event);
}

void NodeGraph::
    mouseMoveEvent(QMouseEvent *event)
{

    if (event->buttons() == Qt::LeftButton)
    {
        // Make sure shift is not being pressed
        if ((event->modifiers() & Qt::ShiftModifier) == 0)
        {
            QPointF difference = _clickPos - mapToScene(event->pos());
            setSceneRect(sceneRect().translated(difference.x(), difference.y()));
        }
    }
    QGraphicsView::mouseMoveEvent(event);
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