#pragma once

#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QList>
#include <QVector>
#include <QSharedPointer>
#include <QPainterPath>
#include <QRectF>

class QGraphicsTextItem;

class Node;
class Port;
class Connection;

typedef QSharedPointer<Node> NodePtr;
typedef QSharedPointer<Connection> ConnectionPtr;
typedef QSharedPointer<Port> PortPtr;

class Scene : public QGraphicsScene
{
public:
    Scene();
};

class Node : public QGraphicsObject
{
    int width;
    int height;
    QGraphicsTextItem *text;

public:
    explicit Node();
    QVector<PortPtr> inPorts;
    QVector<PortPtr> outPorts;

    QRectF
    boundingRect() const override;

protected:
    void
    paint(QPainter *painter,
          QStyleOptionGraphicsItem const *option,
          QWidget *widget = 0) override;

    // void
    // mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    // void
    // mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

    // void
    // mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void
    hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;

    void
    hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

    // void
    // hoverMoveEvent(QGraphicsSceneHoverEvent *) override;

    // void
    // mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
};

class Port : public QGraphicsObject
{
public:
    NodePtr node;
    QVector<ConnectionPtr> connections;

    QString name;
};

class Connection : public QGraphicsObject
{
    PortPtr startPort;
    PortPtr endPort;

public:
    void updatePositions();
};