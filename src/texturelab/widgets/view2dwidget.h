#pragma once

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QMainWindow>
#include <QWidget>

class QWidget;
class QWheelEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QWheelEvent;
class QShowEvent;
class QGraphicsScene;
class QStyleOptionGraphicsItem;

class TextureProject;
class TextureNode;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureNode> TextureNodePtr;
class View2DGraph;

class NodePreviewGraphicsItem;

class View2DWidget : public QMainWindow {
public:
    View2DWidget();

    void setSelectedNode(const TextureNodePtr& node);
    void clearSelection();

    void reRenderNode();

    View2DGraph* graph = nullptr;

    virtual ~View2DWidget();
};

class View2DGraph : public QGraphicsView {
    Q_OBJECT

    TextureNodePtr node;
    QGraphicsScene* _scene;

public:
    View2DGraph(QWidget* parent);

    void scaleUp();
    void scaleDown();

    void setSelectedNode(const TextureNodePtr& node);
    void clearSelection();

protected:
    // view manipulation
    void wheelEvent(QWheelEvent* event) override;
    // void keyPressEvent(QKeyEvent* event) override;
    // void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void drawBackground(QPainter* painter, const QRectF& r) override;

private:
    QPointF _clickPos;
    NodePreviewGraphicsItem* preview = nullptr;
};

class NodePreviewGraphicsItem : public QGraphicsItem {
    TextureNodePtr node;

public:
    NodePreviewGraphicsItem();
    void setNode(const TextureNodePtr& node);
    void clearNode();
    QRectF boundingRect() const override;

protected:
    void paint(QPainter* painter, QStyleOptionGraphicsItem const* option,
               QWidget* widget = 0) override;
};