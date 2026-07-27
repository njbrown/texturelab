#include "frame.h"
#include "nodetheme.h"
#include "scene.h"
#include <QApplication>
#include <QCursor>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QUuid>
#include <QtMath>

namespace nodegraph {

Frame::Frame()
    : QGraphicsObject(), _id(QUuid::createUuid().toString()), _title("Frame"),
      _description(""), _showTitle(true),
      _color(25, 0, 51) // RGB(0.1, 0, 0.2) * 255
      ,
      _frameRect(0, 0, 500, 300), _isHovered(false), _isDragged(false),
      _dragMode(DragMode::None), _xResize(0), _yResize(0)
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setAcceptHoverEvents(true);
    setZValue(-1000); // Behind nodes by default
}

FramePtr Frame::create() { return FramePtr(new Frame()); }

Frame::~Frame() {}

int Frame::type() const { return (int)SceneItemType::Frame; }

void Frame::setTitle(const QString& title)
{
    _title = title;
    update();
}

void Frame::setColor(const QColor& color)
{
    _color = color;
    update();
}

void Frame::setFrameRect(const QRectF& rect)
{
    prepareGeometryChange();
    _frameRect = rect;
    update();
}

void Frame::setSize(qreal width, qreal height)
{
    prepareGeometryChange();
    _frameRect.setWidth(qMax(width, MIN_WIDTH));
    _frameRect.setHeight(qMax(height, MIN_HEIGHT));
    update();
}

void Frame::addNode(NodePtr node)
{
    if (!_nodes.contains(node)) {
        _nodes.append(node);
    }
}

void Frame::removeNode(NodePtr node) { _nodes.removeAll(node); }

void Frame::clearNodes() { _nodes.clear(); }

QRectF Frame::boundingRect() const
{
    return _frameRect.adjusted(-5, -HANDLE_SIZE - 5, 5, 5);
}

QVector<ResizeRegion> Frame::getFrameRegions() const
{
    QVector<ResizeRegion> regions;
    qreal h = RESIZE_HANDLE_SIZE;
    qreal w = _frameRect.width();
    qreal height = _frameRect.height();

    // Top handle bar
    regions.append(
        {QRectF(_frameRect.x(), _frameRect.y() - HANDLE_SIZE, w, HANDLE_SIZE),
         DragMode::HandleTop, 0, 0, Qt::SizeAllCursor});

    // Top-left corner
    regions.append({QRectF(_frameRect.x(), _frameRect.y(), h, h),
                    DragMode::ResizeTopLeft, -1, -1, Qt::SizeFDiagCursor});

    // Top-right corner
    regions.append({QRectF(_frameRect.x() + w - h, _frameRect.y(), h, h),
                    DragMode::ResizeTopRight, 1, -1, Qt::SizeBDiagCursor});

    // Bottom-left corner
    regions.append({QRectF(_frameRect.x(), _frameRect.y() + height - h, h, h),
                    DragMode::ResizeBottomLeft, -1, 1, Qt::SizeBDiagCursor});

    // Bottom-right corner
    regions.append(
        {QRectF(_frameRect.x() + w - h, _frameRect.y() + height - h, h, h),
         DragMode::ResizeBottomRight, 1, 1, Qt::SizeFDiagCursor});

    // Top edge
    regions.append({QRectF(_frameRect.x() + h, _frameRect.y(), w - 2 * h, h),
                    DragMode::ResizeTop, 0, -1, Qt::SizeVerCursor});

    // Bottom edge
    regions.append(
        {QRectF(_frameRect.x() + h, _frameRect.y() + height - h, w - 2 * h, h),
         DragMode::ResizeBottom, 0, 1, Qt::SizeVerCursor});

    // Left edge
    regions.append(
        {QRectF(_frameRect.x(), _frameRect.y() + h, h, height - 2 * h),
         DragMode::ResizeLeft, -1, 0, Qt::SizeHorCursor});

    // Right edge
    regions.append(
        {QRectF(_frameRect.x() + w - h, _frameRect.y() + h, h, height - 2 * h),
         DragMode::ResizeRight, 1, 0, Qt::SizeHorCursor});

    return regions;
}

DragMode Frame::getHitRegion(const QPointF& pos) const
{
    QVector<ResizeRegion> regions = getFrameRegions();
    for (const ResizeRegion& region : regions) {
        if (region.rect.contains(pos)) {
            return region.dragMode;
        }
    }
    return DragMode::None;
}

Qt::CursorShape Frame::getCursorForDragMode(DragMode mode) const
{
    QVector<ResizeRegion> regions = getFrameRegions();
    for (const ResizeRegion& region : regions) {
        if (region.dragMode == mode) {
            return region.cursor;
        }
    }
    return Qt::ArrowCursor;
}

QVector<NodePtr> Frame::getNodesInFrame() const
{
    QVector<NodePtr> nodesInFrame;

    if (!scene()) {
        return nodesInFrame;
    }

    // Get the frame's bounding rectangle in scene coordinates
    QRectF frameSceneRect = mapRectToScene(_frameRect);

    // Get all items in the scene
    QList<QGraphicsItem*> items = scene()->items(frameSceneRect);

    // Filter for Node items that are fully or partially within the frame
    for (QGraphicsItem* item : items) {
        // Check if this is a Node (type() returns SceneItemType::Node)
        if (item->type() == (int)SceneItemType::Node) {
            Node* nodePtr = qgraphicsitem_cast<Node*>(item);
            if (nodePtr) {
                // Get the node's bounding rect in scene coordinates
                QRectF nodeBounds = nodePtr->sceneBoundingRect();

                // Check if the node is fully contained within the frame
                if (frameSceneRect.contains(nodeBounds)) {
                    // Find the shared pointer from the scene
                    Scene* scenePtr = static_cast<Scene*>(scene());
                    if (scenePtr) {
                        QString nodeId = nodePtr->id();
                        NodePtr sharedNode = scenePtr->getNodeById(nodeId);
                        if (sharedNode) {
                            nodesInFrame.append(sharedNode);
                        }
                    }
                }
            }
        }
    }

    return nodesInFrame;
}

void Frame::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                  QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    // Draw top handle bar
    QColor handleColor = _color.lighter(150);
    QRectF handleRect(_frameRect.x(), _frameRect.y() - HANDLE_SIZE,
                      _frameRect.width(), HANDLE_SIZE);
    painter->fillRect(handleRect, handleColor);

    // Draw title if enabled
    if (_showTitle && !_title.isEmpty()) {
        QFont font("Arial", 10, QFont::Bold);
        painter->setFont(font);

        // shadow pass
        painter->setPen(ntColor(Tokens::NodeBorder, 160));
        painter->drawText(handleRect.translated(1, 1), Qt::AlignCenter, _title);

        // text pass
        painter->setPen(ntColor(Tokens::NodeTitle));
        painter->drawText(handleRect, Qt::AlignCenter, _title);
    }

    // Draw frame border
    QPen borderPen;
    if (isSelected()) {
        borderPen.setColor(ntColor(Tokens::FrameSelect)); // themed "selected" accent
        borderPen.setWidth(2);
    }
    else {
        borderPen.setColor(_color.lighter(120));
        borderPen.setWidth(1);
    }
    painter->setPen(borderPen);

    // Draw semi-transparent background
    QColor bgColor = _color;
    bgColor.setAlpha(50); // Semi-transparent
    painter->setBrush(bgColor);
    painter->drawRect(_frameRect);

    // Draw resize handles when selected
    if (isSelected()) {
        painter->setBrush(QColor(100, 100, 100, 100));
        painter->setPen(Qt::NoPen);

        qreal h = RESIZE_HANDLE_SIZE;
        qreal w = _frameRect.width();
        qreal height = _frameRect.height();

        // Corner handles (small squares)
        painter->drawRect(_frameRect.x(), _frameRect.y(), h, h);
        painter->drawRect(_frameRect.x() + w - h, _frameRect.y(), h, h);
        painter->drawRect(_frameRect.x(), _frameRect.y() + height - h, h, h);
        painter->drawRect(_frameRect.x() + w - h, _frameRect.y() + height - h,
                          h, h);
    }
}

void Frame::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF pos = event->pos();
        _dragMode = getHitRegion(pos);
        _isDragged = true;
        _dragStartPos = event->scenePos();
        _dragStartRect = _frameRect;

        // Find resize direction
        QVector<ResizeRegion> regions = getFrameRegions();
        for (const ResizeRegion& region : regions) {
            if (region.dragMode == _dragMode) {
                _xResize = region.xResizeDir;
                _yResize = region.yResizeDir;
                break;
            }
        }

        // Capture nodes geometrically within the frame
        // Skip if Alt key is pressed (allows moving frame without nodes)
        if (!(event->modifiers() & Qt::AltModifier)) {
            _nodeDragStartPositions.clear();

            // Get all nodes currently within the frame's bounds
            QVector<NodePtr> nodesInBounds = getNodesInFrame();

            // Store their starting positions
            for (NodePtr node : nodesInBounds) {
                if (node) {
                    _nodeDragStartPositions[node] = node->pos();
                }
            }
        }

        event->accept();
    }

    QGraphicsObject::mousePressEvent(event);
}

void Frame::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (_isDragged && event->buttons() & Qt::LeftButton) {
        QPointF delta = event->scenePos() - _dragStartPos;

        if (_dragMode == DragMode::HandleTop) {
            // Move the frame and all captured nodes
            setPos(pos() + delta);

            for (auto it = _nodeDragStartPositions.begin();
                 it != _nodeDragStartPositions.end(); ++it) {
                NodePtr node = it.key();
                if (node) {
                    // node->setPos(it.value() + delta);
                    node->moveBy(delta.x(), delta.y());
                }
            }

            _dragStartPos = event->scenePos();
        }
        else if (_dragMode != DragMode::None) {
            // Resize mode
            prepareGeometryChange();

            QRectF newRect = _dragStartRect;

            // Handle horizontal resize
            if (_xResize == -1) { // Left edge
                qreal newX = _dragStartRect.x() + delta.x();
                qreal newWidth = _dragStartRect.width() - delta.x();
                if (newWidth >= MIN_WIDTH) {
                    newRect.setX(newX);
                    newRect.setWidth(newWidth);
                }
            }
            else if (_xResize == 1) { // Right edge
                qreal newWidth = _dragStartRect.width() + delta.x();
                newRect.setWidth(qMax(newWidth, MIN_WIDTH));
            }

            // Handle vertical resize
            if (_yResize == -1) { // Top edge
                qreal newY = _dragStartRect.y() + delta.y();
                qreal newHeight = _dragStartRect.height() - delta.y();
                if (newHeight >= MIN_HEIGHT) {
                    newRect.setY(newY);
                    newRect.setHeight(newHeight);
                }
            }
            else if (_yResize == 1) { // Bottom edge
                qreal newHeight = _dragStartRect.height() + delta.y();
                newRect.setHeight(qMax(newHeight, MIN_HEIGHT));
            }

            _frameRect = newRect;
            update();
        }

        event->accept();
        return;
    }

    QGraphicsObject::mouseMoveEvent(event);
}

void Frame::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        _isDragged = false;
        _dragMode = DragMode::None;
        _xResize = 0;
        _yResize = 0;
        _nodeDragStartPositions.clear();
        setCursor(Qt::ArrowCursor);
        event->accept();
    }

    QGraphicsObject::mouseReleaseEvent(event);
}

void Frame::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    _isHovered = true;
    update();
    QGraphicsObject::hoverEnterEvent(event);
}

void Frame::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    _isHovered = false;
    setCursor(Qt::ArrowCursor);
    update();
    QGraphicsObject::hoverLeaveEvent(event);
}

void Frame::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    QPointF pos = event->pos();
    DragMode mode = getHitRegion(pos);

    if (mode != DragMode::None) {
        Qt::CursorShape cursor = getCursorForDragMode(mode);
        setCursor(cursor);
    }
    else {
        setCursor(Qt::ArrowCursor);
    }

    QGraphicsObject::hoverMoveEvent(event);
}

} // namespace nodegraph
