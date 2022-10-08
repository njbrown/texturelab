#include "scene.h"
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QUuid>

namespace nodegraph {

Scene::Scene() : QGraphicsScene()
{
    this->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

ScenePtr Scene::create() { return ScenePtr(new Scene()); }

void Scene::addNode(NodePtr node)
{
    this->addItem(node.data());
    nodes[node->id()] = node;
}

void Scene::connectNodes(NodePtr leftNode, QString leftOutputName,
                         NodePtr rightNode, QString rightInputName)
{
    auto leftPort = leftNode->getOutPortByName(leftOutputName);
    qDebug() << rightNode->getInPorts();
    auto rightPort = rightNode->getInPortByName(rightInputName);

    // create new connection item from ports
    auto conn = new Connection();
    conn->startPort = leftPort;
    conn->endPort = rightPort;
    conn->updatePosFromPorts();
    conn->updatePathFromPositions();

    ConnectionPtr connPtr(conn);

    // also add them to the ports
    leftPort->addConnection(connPtr);
    rightPort->addConnection(connPtr);

    this->addItem(conn);
}

NodePtr Scene::getNodeById(QString id) { return nodes[id]; }

void Scene::removeNode(NodePtr node)
{
    // gather connections
    QList<ConnectionPtr> cons;
    for (auto port : node->getInPorts()) {
        cons.append(port->connections);
    }

    for (auto port : node->getOutPorts()) {
        cons.append(port->connections);
    }

    // remove connections
    for (auto con : cons) {
        this->removeConnection(con);
    }

    // remove node
    node->hide(); // fix display cache issue
    this->removeItem(node.data());

    // reshow here in case i forget when re-adding node for
    // undo-redo
    node->show();
}

void Scene::removeConnection(ConnectionPtr con)
{
    con->startPort->removeConnection(con);
    con->endPort->removeConnection(con);

    this->removeItem(con.data());
}

Scene::~Scene()
{
    // remove all items manually otherwise
    // smart point destructor of some items
    // will cause segfault when cleaning up
    auto items = this->items();
    for (auto item : items)
        this->removeItem(item);
}

Node::Node()
{
    width = 100;
    height = 100;
    isHovered = false;

    defaultBorderColor = QColor(0, 0, 0);
    highlightBorderColor = QColor(120, 120, 120);
    selectedBorderColor = QColor(200, 200, 200);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

    setCursor(Qt::ClosedHandCursor);

    text = new QGraphicsTextItem(this);

    text->setPos(0, 0);
    text->setTextWidth(100);
    text->setDefaultTextColor(QColor(255, 255, 255));

    // center title
    setName("Title");

    text->document()->setDocumentMargin(2);

    QFont font = text->font();
    font.setWeight(QFont::Bold);
    font.setPixelSize(12);
    text->setFont(font);

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect;
    effect->setBlurRadius(20);
    effect->setXOffset(0);
    effect->setYOffset(0);
    effect->setColor(QColor(00, 00, 00, 70));
    setGraphicsEffect(effect);

    setAcceptHoverEvents(true);
    // setAcceptDrops(true);
}

NodePtr Node::create() { return NodePtr(new Node()); }

void Node::setName(QString name)
{
    this->name = name;
    text->setPlainText(name);

    QTextBlockFormat format;
    format.setAlignment(Qt::AlignCenter);
    QTextCursor cursor = text->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(format);
    cursor.clearSelection();
    text->setTextCursor(cursor);
}

const QVector<PortPtr> Node::getInPorts() const { return inPorts; }

const QVector<PortPtr> Node::getOutPorts() const { return outPorts; }

void Node::addInPort(QString name)
{
    PortPtr port(new Port(this));
    port->name = name;
    port->portType = PortType::In;
    port->node = this->sharedFromThis();
    inPorts.append(port);

    // top and bottom padding for sockets
    const int pad = inPorts.count() < 5 ? 10 : 0;

    // sort in sockets
    int incr = (this->height - pad * 2) / inPorts.count();
    int mid = incr / 2.0;
    int i = 0;
    for (auto port : inPorts) {
        int y = pad + i * incr + mid;
        int x = 0;
        port->setCenter(x, y);
        i++;
    }
}

void Node::addOutPort(QString name)
{
    PortPtr port(new Port(this));
    port->name = name;
    port->portType = PortType::Out;
    port->node = this->sharedFromThis();
    outPorts.append(port);

    // top and bottom padding for sockets
    const int pad = outPorts.count() < 5 ? 10 : 0;

    // sort in sockets
    int incr = (this->height - pad * 2) / outPorts.count();
    int mid = incr / 2.0;
    int i = 0;
    for (auto port : outPorts) {
        int y = pad + i * incr + mid;
        int x = width;
        port->setCenter(x, y);
        i++;
    }
}

PortPtr Node::getPortById(QString id)
{
    for (auto port : inPorts) {
        if (port->id() == id)
            return port;
    }

    for (auto port : outPorts) {
        if (port->id() == id)
            return port;
    }

    Q_ASSERT(false);
}

PortPtr Node::getInPortByName(QString name)
{
    for (auto port : inPorts) {
        if (port->name == name)
            return port;
    }

    Q_ASSERT(false);
}

PortPtr Node::getOutPortByName(QString name)
{
    for (auto port : outPorts) {
        if (port->name == name)
            return port;
    }

    Q_ASSERT(false);
}

QRectF Node::boundingRect() const { return QRectF(0, 0, 100, 100); }

// void Node::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
// {
//     QGraphicsObject::mouseMoveEvent(event);
// }

void Node::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    this->text->hide();
    this->isHovered = true;
    QGraphicsObject::hoverEnterEvent(event);
}

void Node::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    this->text->show();
    this->isHovered = false;
    QGraphicsObject::hoverLeaveEvent(event);
}

void Node::paint(QPainter* painter, QStyleOptionGraphicsItem const* option,
                 QWidget* widget)
{
    const int titleHeight = 20;
    const int nodeWidth = width;
    const int nodeHeight = height;
    const int titleRadius = 4;
    const QColor titleColor(0, 0, 0);

    auto rect = boundingRect();

    QColor borderColor;
    if (isSelected())
        borderColor = this->selectedBorderColor;
    else if (isHovered)
        borderColor = this->highlightBorderColor;
    else
        borderColor = this->defaultBorderColor;

    // not really needed
    // painter->setClipRect(option->exposedRect);

    // smooth rendering
    // painter->setRenderHint(QPainter::Antialiasing);
    // painter->setRenderHint(QPainter::TextAntialiasing);

    // title tab
    // QPainterPath titlePath;
    // titlePath.setFillRule(Qt::WindingFill);
    // titlePath.addRect(0, 10, width, titleHeight - 10);
    // titlePath.addRoundedRect(0, 0, nodeWidth, titleHeight, titleRadius,
    // titleRadius); painter->fillPath(titlePath, QBrush(QColor(255, 255,
    // 255)));

    // // draw text node seperator
    // QPainterPath block;
    // block.setFillRule(Qt::WindingFill);
    // block.addRect(0, titleHeight, nodeWidth, 3);
    // painter->fillPath(block, QBrush(QColor(30, 30, 30, 160)));

    // QPen pen(QColor(00, 00, 00, 250), .5);
    // QPen pen(borderColor, .5);
    // painter->setPen(pen);

    // background
    QPainterPath bgPath;
    bgPath.setFillRule(Qt::WindingFill);
    bgPath.addRoundedRect(0, 0, nodeWidth, nodeHeight, titleRadius,
                          titleRadius);
    painter->fillPath(bgPath, QBrush(QColor(10, 10, 10, 255)));

    // draw border
    painter->setPen(QPen(borderColor, 3));
    painter->drawRoundedRect(rect, titleRadius, titleRadius);

    // draw highlight
}

Node::~Node()
{
    // remove ownership from scene else scene will try to
    // clean it up after it's been deleted
    if (this->scene())
        this->scene()->removeItem(this);
}

QString Port::id() const { return _id; }

Port::Port(QGraphicsObject* parent) : QGraphicsObject(parent)
{
    setCursor(Qt::ClosedHandCursor);

    // this->setFlag(QGraphicsItem::ItemIsSelectable, false);
    this->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

    _radius = 7;
    name = "";
    portType = PortType::In;
    _id = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QRectF Port::boundingRect() const
{
    // return QRectF(-_radius, -_radius, _radius * 2, _radius * 2);

    // add extra space for hit testing
    return QRectF(-_radius * 2, -_radius * 2, _radius * 4, _radius * 4);
}

QRectF Port::actualRect() const
{
    return QRectF(-_radius, -_radius, _radius * 2, _radius * 2);
}

void Port::setCenter(float x, float y)
{
    // auto rect = this->boundingRect();
    // setPos(QPointF(x - rect.x() / 2, y - rect.y() / 2));
    setPos(QPointF(x, y));
}

QVariant Port::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemScenePositionHasChanged) {
        for (auto con : connections) {
            con->updatePosFromPorts();
            con->updatePathFromPositions();
        }
    }
    return value;
}

void Port::removeConnection(ConnectionPtr con)
{
    // todo: make sure this does what it's supposed to do
    connections.removeOne(con);
}

void Port::paint(QPainter* painter, QStyleOptionGraphicsItem const* option,
                 QWidget* widget)
{
    auto rect = actualRect();

    QPen pen(QColor(00, 00, 00, 250), 1.0f);
    painter->setPen(pen);

    // background
    QPainterPath bgPath;
    bgPath.setFillRule(Qt::WindingFill);
    // bgPath.addRoundedRect(-_radius, _radius, rect.width(), rect.height(),
    // rect.width() / 2, rect.height() / 2);
    bgPath.addRoundedRect(rect, _radius, _radius);
    painter->fillPath(bgPath, QBrush(QColor(170, 170, 170, 255)));

    // draw border
    painter->setPen(QPen(QColor(0, 0, 0), 3));
    painter->drawRoundedRect(rect, rect.width() / 2, rect.height() / 2);
}

Port::~Port()
{
    // remove ownership from scene else scene will try to
    // clean it up after it's been deleted
    if (this->scene())
        this->scene()->removeItem(this);
}

Connection::Connection()
{
    pos1 = QPointF(0, 0);
    pos2 = QPointF(0, 0);

    connectState = ConnectionState::Complete;

    auto pen = QPen(QColor(200, 200, 200));
    pen.setBrush(QColor(50, 150, 250));
    pen.setCapStyle(Qt::RoundCap);
    pen.setWidth(lineThickness);
    setPen(pen);
}

void Connection::updatePosFromPorts()
{
    pos1 = startPort->scenePos();
    pos2 = endPort->scenePos();
}

void Connection::updatePathFromPositions()
{
    p = new QPainterPath;
    p->moveTo(pos1);

    qreal dx = pos2.x() - pos1.x();
    qreal dy = pos2.y() - pos1.y();

    QPointF ctr1(pos1.x() + dx * 0.5, pos1.y());
    QPointF ctr2(pos2.x() - dx * 0.5, pos2.y());

    p->cubicTo(ctr1, ctr2, pos2);
    p->setFillRule(Qt::OddEvenFill);

    setPath(*p);
}

void Connection::paint(QPainter* painter,
                       const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->save();

    if (connectState == ConnectionState::Dragging) {
        QPen pen(QColor(150, 150, 150), lineThickness);
        pen.setStyle(Qt::DashLine);
        pen.setDashOffset(4);
        painter->setPen(pen);
        painter->drawPath(*p);

        painter->setPen(QPen(QColor(0, 0, 0), 3));
        painter->setBrush(QBrush(QColor(150, 150, 150)));
        painter->drawEllipse(pos1, 7, 7);

        painter->setPen(Qt::NoPen);
        painter->drawEllipse(pos2, 6, 6);
    }
    if (connectState == ConnectionState::Complete) {
        // create gradient for line
        QPen pen(QColor(170, 170, 170), lineThickness);
        painter->setPen(pen);
        painter->drawPath(*p);

        painter->setPen(QPen(QColor(0, 0, 0), 3));
        painter->setBrush(QBrush(QColor(170, 170, 170)));
        painter->drawEllipse(pos1, 7, 7);
        painter->drawEllipse(pos2, 7, 7);
    }

    painter->restore();

    Q_UNUSED(option);
    Q_UNUSED(widget);
}

Connection::~Connection()
{
    // remove ownership from scene else scene will try to
    // clean it up after it's been deleted
    if (this->scene())
        this->scene()->removeItem(this);
}

} // namespace nodegraph