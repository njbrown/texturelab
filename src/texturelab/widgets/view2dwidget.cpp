#include "view2dwidget.h"
#include <QLayout>

#include <QtGui/QBrush>
#include <QtGui/QPen>
#include <QtWidgets/QMenu>

#include <QtCore/QPointF>
#include <QtCore/QRectF>

#include <QGraphicsView>
#include <QWheelEvent>

#include <QOpenGLFramebufferObject>
#include <QOpenGLWidget>
#include <QPaintEngine>

#include "./graphics/texturerenderer.h"
#include "./models.h"
#include "./utils.h"

const QColor BackgroundColor(53, 53, 53);
const QColor FineGridColor(60, 60, 60);
const QColor CoarseGridColor(25, 25, 25);

View2DWidget::View2DWidget() : QMainWindow()
{
    graph = new View2DGraph(this);
    this->setCentralWidget(graph);
}

void View2DWidget::setSelectedNode(const TextureNodePtr& node)
{
    this->node = node;
    this->graph->setSelectedNode(node);
}

void View2DWidget::clearSelection() {}

void View2DWidget::reRenderNode() { this->graph->scene()->invalidate(); }

void View2DWidget::setTextureRenderer(TextureRenderer* renderer)
{
    connect(renderer, &TextureRenderer::thumbnailGenerated,
            [=](const QString& nodeId, GLint texId, const QPixmap& pixmap) {
                if (!!node && node->id == nodeId) {
                    this->graph->scene()->invalidate();
                }
            });
}

View2DWidget::~View2DWidget() { delete graph; }

// GRAPH
View2DGraph::View2DGraph(QWidget* parent) : QGraphicsView(parent)
{
    // https://doc.qt.io/qt-6.2/graphicsview.html#opengl-rendering
    auto gl = new QOpenGLWidget();
    QSurfaceFormat format;
    format.setSamples(4);
    gl->setFormat(format);
    this->setViewport(gl);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHint(QPainter::Antialiasing);

    setBackgroundBrush(QColor(33, 33, 33));

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    _scene = new QGraphicsScene(this);
    this->setScene(_scene);

    preview = new NodePreviewGraphicsItem();
    this->_scene->addItem(preview);
    // this->fitInView(preview, Qt::KeepAspectRatio);
    scale(0.3, 0.3);
    preview->hide();
}

// view manipulation
void View2DGraph::wheelEvent(QWheelEvent* event)
{
    QPoint delta = event->angleDelta();

    if (delta.y() == 0) {
        event->ignore();
        return;
    }

    double const d = delta.y() / std::abs(delta.y());

    if (d > 0.0)
        scaleUp();
    else
        scaleDown();
};

void View2DGraph::scaleUp()
{
    double const step = 1.2;
    double const factor = std::pow(step, 1.0);

    QTransform t = transform();

    if (t.m11() > 2.0)
        return;

    scale(factor, factor);
}

void View2DGraph::scaleDown()
{
    double const step = 1.2;
    double const factor = std::pow(step, -1.0);

    scale(factor, factor);
}

// void View2DGraph::keyPressEvent(QKeyEvent* event){};
// void View2DGraph::keyReleaseEvent(QKeyEvent* event){};
void View2DGraph::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton &&
        scene()->mouseGrabberItem() == nullptr) {
        _clickPos = mapToScene(event->pos());
        setDragMode(QGraphicsView::NoDrag);
    }
    QGraphicsView::mousePressEvent(event);
}

void View2DGraph::mouseMoveEvent(QMouseEvent* event)
{

    if (event->buttons() == Qt::MiddleButton) {
        QPointF difference = _clickPos - mapToScene(event->pos());
        setSceneRect(sceneRect().translated(difference.x(), difference.y()));
    }
    QGraphicsView::mouseMoveEvent(event);
}

void View2DGraph::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void View2DGraph::setSelectedNode(const TextureNodePtr& node)
{
    this->preview->setNode(node);
    this->scene()->invalidate();
};

void View2DGraph::clearSelection(){};

void View2DGraph::drawBackground(QPainter* painter, const QRectF& r)
{
    auto type = painter->paintEngine()->type();
    if (type != QPaintEngine::OpenGL && type != QPaintEngine::OpenGL2) {
        qWarning() << "background paint engine needs to be OPENGL!";
        // return;
    }

    QGraphicsView::drawBackground(painter, r);
    painter->setRenderHint(QPainter::Antialiasing);

    auto drawGrid = [&](double gridStep) {
        QRect windowRect = rect();
        QPointF tl = mapToScene(windowRect.topLeft());
        QPointF br = mapToScene(windowRect.bottomRight());

        double left = std::floor(tl.x() / gridStep - 0.5);
        double right = std::floor(br.x() / gridStep + 1.0);
        double bottom = std::floor(tl.y() / gridStep - 0.5);
        double top = std::floor(br.y() / gridStep + 1.0);

        // vertical lines
        for (int xi = int(left); xi <= int(right); ++xi) {
            QLineF line(xi * gridStep, bottom * gridStep, xi * gridStep,
                        top * gridStep);

            painter->drawLine(line);
        }

        // horizontal lines
        for (int yi = int(bottom); yi <= int(top); ++yi) {
            QLineF line(left * gridStep, yi * gridStep, right * gridStep,
                        yi * gridStep);
            painter->drawLine(line);
        }
    };

    QBrush bBrush = backgroundBrush();

    // QPen pfine(FineGridColor, 1.0);

    // painter->setPen(pfine);
    // drawGrid(15);

    QPen p(CoarseGridColor, 1.0);
    painter->setPen(p);
    drawGrid(1000);
};

// NODE PREVIEW
NodePreviewGraphicsItem::NodePreviewGraphicsItem() {}

QRectF NodePreviewGraphicsItem::boundingRect() const
{
    return QRectF(0, 0, 1000, 1000);
}

void NodePreviewGraphicsItem::setNode(const TextureNodePtr& node)
{
    this->node = node;
    if (!!node)
        this->show();
}
void NodePreviewGraphicsItem::clearNode() { this->node.reset(); }

void NodePreviewGraphicsItem::paint(QPainter* painter,
                                    QStyleOptionGraphicsItem const* option,
                                    QWidget* widget)
{
    // // https://doc.qt.io/qt-5/qpainter.html#beginNativePainting
    // https://github.com/liff-engineer/WeeklyARTS/blob/d8605aa3bfb2641d2a13621262024a1edff7b661/2018_9_4/Mixin2D%263DinQt.md
    auto type = painter->paintEngine()->type();
    if (type != QPaintEngine::OpenGL && type != QPaintEngine::OpenGL2) {
        qWarning() << "Paint engine needs to be OPENGL!";
        // return;
    }

    auto rect = boundingRect();

    if (!!node) {
        // // https://doc.qt.io/qt-5/qpainter.html#beginNativePainting
        painter->beginNativePainting();

        // glColor4f(1.0f, 0.0f, 0.0f, 1.0);
        glEnable(GL_TEXTURE_2D);

        glActiveTexture(0);
        glBindTexture(GL_TEXTURE_2D, node->texture->texture());
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(0, 0);

        glTexCoord2f(1, 0);
        glVertex2f(rect.width(), 0);

        glTexCoord2f(1, 1);
        glVertex2f(rect.width(), rect.height());

        glTexCoord2f(0, 1);
        glVertex2f(0, rect.height());
        glEnd();

        painter->endNativePainting();
    }
}