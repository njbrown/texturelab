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

    bool isHovered;

public:
    explicit Node();
    QVector<PortPtr> inPorts;
    QVector<PortPtr> outPorts;

    void addInPort(QString name);
    void addOutPort(QString name);

    QRectF
    boundingRect() const override;

    bool hovered() const { return isHovered; };

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
    QGraphicsTextItem *text;
    int _radius;

public:
    NodePtr node;
    QVector<ConnectionPtr> connections;

    QString name;

    int radius() const { return _radius; }

    Port(QGraphicsObject *parent);

    QRectF
    boundingRect() const override;

    void setCenter(float x, float y);

protected:
    void
    paint(QPainter *painter,
          QStyleOptionGraphicsItem const *option,
          QWidget *widget = 0) override;
};

class Connection : public QGraphicsObject
{
    PortPtr startPort;
    PortPtr endPort;

public:
    void updatePositions();
};