#pragma once

#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QList>
#include <QVector>
#include <QSharedPointer>
#include <QRectF>
#include <QGraphicsPathItem>

class QGraphicsTextItem;
class QPainterPath;

class Node;
class Port;
class Connection;

typedef QSharedPointer<Node> NodePtr;
typedef QSharedPointer<Connection> ConnectionPtr;
typedef QSharedPointer<Port> PortPtr;

enum class SceneItemType : int
{
    Node = 1,
    Port = 2,
    Connection = 3
};

class Scene : public QGraphicsScene, public QEnableSharedFromThis<Scene>
{
public:
    Scene();
    void addNode(NodePtr node);
    void connectNodes(NodePtr leftNode, QString leftOutputName, NodePtr rightNode, QString rightInputName);
};

class Node : public QGraphicsObject, public QEnableSharedFromThis<Node>
{
    int width;
    int height;
    QGraphicsTextItem *text;

    bool isHovered;
    // bool isSelected;

    QColor defaultBorderColor;
    QColor highlightBorderColor;
    QColor selectedBorderColor;

public:
    explicit Node();
    QVector<PortPtr> inPorts;
    QVector<PortPtr> outPorts;

    void addInPort(QString name);
    void addOutPort(QString name);

    PortPtr getPortById(QString id);

    PortPtr getInPortByName(QString name);
    PortPtr getOutPortByName(QString name);

    QRectF
    boundingRect() const override;

    bool hovered() const { return isHovered; };

    virtual int type() const override { return (int)SceneItemType::Node; }

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

enum class PortType : int
{
    Invalid = 0,
    In = 1,
    Out = 2
};

class Port : public QGraphicsObject, public QEnableSharedFromThis<Port>
{
    QGraphicsTextItem *text;
    int _radius;
    QString _id;

public:
    NodePtr node;
    QVector<ConnectionPtr> connections;

    PortType portType;
    QString name;

    QString id() const;

    int radius() const { return _radius; }

    void addConnection(ConnectionPtr con) { connections.append(con); }

    // todo: implement
    void removeConnection(ConnectionPtr con);

    Port(QGraphicsObject *parent);

    QRectF
    boundingRect() const override;

    QRectF
    actualRect() const;

    void setCenter(float x, float y);

    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    virtual int type() const override { return (int)SceneItemType::Port; }

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

class Connection : public QGraphicsPathItem, public QEnableSharedFromThis<Connection>
{
    friend class Scene;

public:
    ConnectionState connectState;

    PortPtr startPort;
    PortPtr endPort;

    QPointF pos1;
    QPointF pos2;

    double lineThickness = 4.0;

    virtual int type() const override { return (int)SceneItemType::Connection; }

public:
    explicit Connection();
    void updatePositions();

    void updatePosFromPorts();
    void updatePathFromPositions();

    QPainterPath *p;

    // virtual int type() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};