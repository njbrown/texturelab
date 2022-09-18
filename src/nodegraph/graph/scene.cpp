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

    text->document()->setDocumentMargin(0);

    QFont font = text->font();
    font.setWeight(QFont::Bold);
    font.setPixelSize(12);
    text->setFont(font);

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect;
    effect->setBlurRadius(12);
    effect->setXOffset(0);
    effect->setYOffset(0);
    effect->setColor(QColor(00, 00, 00, 40));
    setGraphicsEffect(effect);

    setAcceptHoverEvents(true);
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
    QGraphicsObject::hoverEnterEvent(event);
}

void Node::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    this->text->show();
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
    painter->fillPath(bgPath, QBrush(QColor(10, 10, 10, 160)));

    // draw border
    painter->setPen(QPen(titleColor, 2));
    painter->drawRoundedRect(rect, titleRadius, titleRadius);

    // draw highlight
}