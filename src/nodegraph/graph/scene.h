#pragma once

#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QList>
#include <QVector>
#include <QSharedPointer>
#include <QPainterPath>
#include <QRectF>
#include <QGraphicsPathItem>

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
    void addNode(NodePtr node);
    void connectNodes(NodePtr leftNode, QString leftOutputName, NodePtr rightNode, QString rightInputName);
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

    PortPtr getInPortByName(QString name);
    PortPtr getOutPortByName(QString name);

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

    void addConnection(ConnectionPtr con) { connections.append(con); }

    // todo: implement
    void removeConnection(ConnectionPtr con);

    Port(QGraphicsObject *parent);

    QRectF
    boundingRect() const override;

    void setCenter(float x, float y);

    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

protected:
    void
    paint(QPainter *painter,
          QStyleOptionGraphicsItem const *option,
          QWidget *widget = 0) override;
};

enum class ConnectionState
{
    Dragging,
    Complete
};

class Connection : public QGraphicsPathItem
{
    friend class Scene;

public:
    ConnectionState connectState;

    PortPtr startPort;
    PortPtr endPort;

    QPointF pos1;
    QPointF pos2;

    double lineThickness = 5.0;

public:
    explicit Connection();
    void updatePositions();

    void updatePosFromPorts();
    void updatePath();

    QPainterPath *p;

    // virtual int type() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};