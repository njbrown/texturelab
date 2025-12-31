#pragma once

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QMainWindow>
#include <QToolBar>
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
class TextureRenderer;

class TextureProject;
class TextureNode;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureNode> TextureNodePtr;
class View2DGraph;

class NodePreviewGraphicsItem;

class View2DWidget : public QMainWindow {
    Q_OBJECT

    TextureNodePtr node;
    QToolBar* toolbar;
    bool showTiled = false;

public:
    View2DWidget();

    void setSelectedNode(const TextureNodePtr& node);
    void clearSelection();

    void setTextureRenderer(TextureRenderer* renderer);

    void reRenderNode();

    View2DGraph* graph = nullptr;

    virtual ~View2DWidget();

private:
    void showToast(const QString& message, int duration = 2000);

private slots:
    void saveTextureAsImage();
    void toggleTileView();
    void recenterView();
    void copyTextureToClipboard();
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
    void updatePreview();
    void clearSelection();

    NodePreviewGraphicsItem* preview = nullptr;

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
};

class NodePreviewGraphicsItem : public QGraphicsItem {
    TextureNodePtr node;
    bool tiled = false;

public:
    NodePreviewGraphicsItem();
    void setNode(const TextureNodePtr& node);
    void clearNode();
    void setTiled(bool tiled);
    QRectF boundingRect() const override;

protected:
    void paint(QPainter* painter, QStyleOptionGraphicsItem const* option,
               QWidget* widget = 0) override;
};