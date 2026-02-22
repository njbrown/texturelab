#pragma once

#include <QGraphicsObject>
#include <QRectF>
#include <QSharedPointer>
#include <QString>

namespace nodegraph {

class Comment;
typedef QSharedPointer<Comment> CommentPtr;

class Comment : public QGraphicsObject, public QEnableSharedFromThis<Comment> {
    Q_OBJECT

    QString _id;
    QString _text;

    static constexpr qreal PADDING = 8.0;
    static constexpr int FONT_SIZE = 14;

    QRectF calcTextRect() const;

public:
    explicit Comment();
    static CommentPtr create();

    const QString& id() const { return _id; }
    void setId(const QString& id) { _id = id; }

    const QString& text() const { return _text; }
    void setText(const QString& text);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    virtual int type() const override;

    virtual ~Comment();
};

} // namespace nodegraph
