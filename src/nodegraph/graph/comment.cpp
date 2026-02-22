#include "comment.h"
#include <QCursor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QStringList>
#include <QUuid>

namespace nodegraph {

Comment::Comment()
    : QGraphicsObject(), _id(QUuid::createUuid().toString()), _text("Comment")
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    // setCursor(Qt::SizeAllCursor);
}

CommentPtr Comment::create() { return CommentPtr(new Comment()); }

Comment::~Comment() {}

void Comment::setText(const QString& text)
{
    prepareGeometryChange();
    _text = text;
    update();
}

QRectF Comment::calcTextRect() const
{
    QFont font("Arial", FONT_SIZE);
    QFontMetrics fm(font);

    const QStringList lines = _text.split('\n');
    qreal maxWidth = 0;
    for (const QString& line : lines) {
        maxWidth = qMax(maxWidth, (qreal)fm.horizontalAdvance(line));
    }

    qreal w = maxWidth + PADDING * 2;
    qreal h = lines.count() * fm.height() + PADDING * 2;
    return QRectF(0, 0, w, h);
}

QRectF Comment::boundingRect() const
{
    return calcTextRect().adjusted(-1, -1, 1, 1);
}

void Comment::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                    QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    const QRectF rect = calcTextRect();

    // Semi-transparent white background
    painter->setBrush(QColor(255, 255, 255, 30));
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(rect, 4, 4);

    // White border
    QPen borderPen(QColor(255, 255, 255, isSelected() ? 220 : 140), 1.0);
    painter->setPen(borderPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(rect, 4, 4);

    // White text
    QFont font("Arial", FONT_SIZE);
    painter->setFont(font);
    painter->setPen(QColor(240, 240, 240));

    QFontMetrics fm(font);
    const QStringList lines = _text.split('\n');
    qreal textY = rect.top() + PADDING + fm.ascent();
    for (const QString& line : lines) {
        painter->drawText(QPointF(rect.left() + PADDING, textY), line);
        textY += fm.height();
    }
}

} // namespace nodegraph
