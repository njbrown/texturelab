#pragma once

#include <QGraphicsObject>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QList>
#include <QMap>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QPixmap>
#include <QRectF>
#include <QSharedPointer>
#include <QVector>
#include <QColor>
#include <QCursor>

namespace nodegraph {

class Node;
typedef QSharedPointer<Node> NodePtr;

class Frame;
typedef QSharedPointer<Frame> FramePtr;

enum class DragMode {
    None,
    HandleTop,
    ResizeLeft,
    ResizeRight,
    ResizeTop,
    ResizeBottom,
    ResizeTopLeft,
    ResizeTopRight,
    ResizeBottomLeft,
    ResizeBottomRight
};

struct ResizeRegion {
    QRectF rect;
    DragMode dragMode;
    int xResizeDir;  // -1 for left, 1 for right, 0 for none
    int yResizeDir;  // -1 for top, 1 for bottom, 0 for none
    Qt::CursorShape cursor;
};

class Frame : public QGraphicsObject, public QEnableSharedFromThis<Frame> {
    Q_OBJECT

private:
    QString _id;
    QString _title;
    QString _description;
    bool _showTitle;
    QColor _color;

    // Frame dimensions
    QRectF _frameRect;

    // Interaction state
    bool _isHovered;
    bool _isDragged;
    DragMode _dragMode;
    int _xResize;
    int _yResize;

    // Drag start state (for undo/redo)
    QPointF _dragStartPos;
    QRectF _dragStartRect;

    // UI dimensions
    static constexpr qreal HANDLE_SIZE = 30.0;
    static constexpr qreal RESIZE_HANDLE_SIZE = 20.0;
    static constexpr qreal MIN_WIDTH = 100.0;
    static constexpr qreal MIN_HEIGHT = 80.0;

    // Contained nodes
    QVector<NodePtr> _nodes;
    QMap<NodePtr, QPointF> _nodeDragStartPositions;

    // Helper methods
    QVector<ResizeRegion> getFrameRegions() const;
    DragMode getHitRegion(const QPointF& pos) const;
    Qt::CursorShape getCursorForDragMode(DragMode mode) const;
    QVector<NodePtr> getNodesInFrame() const;

public:
    explicit Frame();
    static FramePtr create();

    // Property getters/setters
    const QString& id() const { return _id; }
    void setId(const QString& id) { _id = id; }

    const QString& title() const { return _title; }
    void setTitle(const QString& title);

    const QString& description() const { return _description; }
    void setDescription(const QString& desc) { _description = desc; }

    bool showTitle() const { return _showTitle; }
    void setShowTitle(bool show) { _showTitle = show; }

    const QColor& color() const { return _color; }
    void setColor(const QColor& color);

    // Frame rectangle methods
    const QRectF& frameRect() const { return _frameRect; }
    void setFrameRect(const QRectF& rect);
    void setSize(qreal width, qreal height);

    // Node management
    void addNode(NodePtr node);
    void removeNode(NodePtr node);
    void clearNodes();
    const QVector<NodePtr>& nodes() const { return _nodes; }

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    virtual int type() const override;

    virtual ~Frame();

protected:
    // Mouse event handlers
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
};

} // namespace nodegraph
