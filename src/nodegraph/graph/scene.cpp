#include "scene.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsDropShadowEffect>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>

Scene::Scene() : QGraphicsScene()
{
}

Node::Node()
{
    width = 100;
    height = 100;
    isHovered = false;

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

    text = new QGraphicsTextItem(this);
    text->setPlainText("Title");

    text->setPos(0, 0);
    text->setTextWidth(100);
    text->setDefaultTextColor(QColor(255, 255, 255));

    // center title
    QTextBlockFormat format;
    format.setAlignment(Qt::AlignCenter);
    QTextCursor cursor = text->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(format);
    cursor.clearSelection();
    text->setTextCursor(cursor);

    text->document()->setDocumentMargin(2);

    QFont font = text->font();
    font.setWeight(QFont::Bold);
    font.setPixelSize(12);
    text->setFont(font);

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect;
    effect->setBlurRadius(20);
    effect->setXOffset(0);
    effect->setYOffset(0);
    effect->setColor(QColor(00, 00, 00, 70));
    setGraphicsEffect(effect);

    setAcceptHoverEvents(true);
}

void Node::addInPort(QString name)
{
    PortPtr port(new Port(this));
    port->name = name;
    inPorts.append(port);
    // port->setParent(this);

    // top and bottom padding for sockets
    const int pad = inPorts.count() < 5 ? 10 : 0;

    // sort in sockets
    int incr = (this->height - pad * 2) / inPorts.count();
    int mid = incr / 2.0;
    int i = 0;
    for (auto port : inPorts)
    {
        int y = pad + i * incr + mid;
        int x = 0;
        port->setCenter(x, y);
        i++;
    }
}

void Node::addOutPort(QString name)
{
}

QRectF
Node::boundingRect() const
{
    return QRectF(0, 0, 100, 100);
}

// void Node::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
// {
//     QGraphicsObject::mouseMoveEvent(event);
// }

void Node::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    this->text->hide();
    this->isHovered = true;
    QGraphicsObject::hoverEnterEvent(event);
}

void Node::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    this->text->show();
    this->isHovered = false;
    QGraphicsObject::hoverLeaveEvent(event);
}

void Node::paint(QPainter *painter,
                 QStyleOptionGraphicsItem const *option,
                 QWidget *widget)
{
    const int titleHeight = 20;
    const int nodeWidth = width;
    const int nodeHeight = height;
    const int titleRadius = 4;
    const QColor titleColor(0, 0, 0);

    auto rect = boundingRect();

    // not really needed
    // painter->setClipRect(option->exposedRect);

    // smooth rendering
    // painter->setRenderHint(QPainter::Antialiasing);
    // painter->setRenderHint(QPainter::TextAntialiasing);

    // title tab
    // QPainterPath titlePath;
    // titlePath.setFillRule(Qt::WindingFill);
    // titlePath.addRect(0, 10, width, titleHeight - 10);
    // titlePath.addRoundedRect(0, 0, nodeWidth, titleHeight, titleRadius, titleRadius);
    // painter->fillPath(titlePath, QBrush(QColor(255, 255, 255)));

    // // draw text node seperator
    // QPainterPath block;
    // block.setFillRule(Qt::WindingFill);
    // block.addRect(0, titleHeight, nodeWidth, 3);
    // painter->fillPath(block, QBrush(QColor(30, 30, 30, 160)));

    QPen pen(QColor(00, 00, 00, 250), .5);
    painter->setPen(pen);

    // background
    QPainterPath bgPath;
    bgPath.setFillRule(Qt::WindingFill);
    bgPath.addRoundedRect(0, 0, nodeWidth, nodeHeight, titleRadius, titleRadius);
    painter->fillPath(bgPath, QBrush(QColor(10, 10, 10, 255)));

    // draw border
    painter->setPen(QPen(titleColor, 3));
    painter->drawRoundedRect(rect, titleRadius, titleRadius);

    // draw highlight
}

Port::Port(QGraphicsObject *parent) : QGraphicsObject(parent)
{
    _radius = 7;
    name = "";
}

QRectF
Port::boundingRect() const
{
    return QRectF(-_radius, -_radius, _radius * 2, _radius * 2);
}

void Port::setCenter(float x, float y)
{
    // auto rect = this->boundingRect();
    // setPos(QPointF(x - rect.x() / 2, y - rect.y() / 2));
    setPos(QPointF(x, y));
}

void Port::paint(QPainter *painter,
                 QStyleOptionGraphicsItem const *option,
                 QWidget *widget)
{
    auto rect = boundingRect();

    QPen pen(QColor(00, 00, 00, 250), 1.0f);
    painter->setPen(pen);

    // background
    QPainterPath bgPath;
    bgPath.setFillRule(Qt::WindingFill);
    // bgPath.addRoundedRect(-_radius, _radius, rect.width(), rect.height(), rect.width() / 2, rect.height() / 2);
    bgPath.addRoundedRect(rect, _radius, _radius);
    painter->fillPath(bgPath, QBrush(QColor(170, 170, 170, 255)));

    // draw border
    painter->setPen(QPen(QColor(0, 0, 0), 3));
    painter->drawRoundedRect(rect, rect.width() / 2, rect.height() / 2);
}