#include "view2dwidget.h"
#include <QLayout>

#include <QtCore/QPropertyAnimation>
#include <QtCore/QTimer>
#include <QtGui/QBrush>
#include <QtGui/QClipboard>
#include <QtGui/QIcon>
#include <QtGui/QImage>
#include <QtGui/QPen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGraphicsOpacityEffect>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>

#include <QtCore/QPointF>
#include <QtCore/QRectF>

#include <QGraphicsView>
#include <QWheelEvent>

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLWidget>
#include <QPaintEngine>
#include <QOpenGLExtraFunctions>

#include "./graphics/texturerenderer.h"
#include "./models.h"
#include "./utils.h"

const QColor BackgroundColor(53, 53, 53);
const QColor FineGridColor(60, 60, 60);
const QColor CoarseGridColor(25, 25, 25);

View2DWidget::View2DWidget() : QMainWindow()
{
    // Create toolbar
    toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(24, 24));
    this->addToolBar(Qt::TopToolBarArea, toolbar);

    // Add save button
    QAction* saveAction =
        toolbar->addAction(QIcon(":/icons/save.svg"), "Save Texture");
    connect(saveAction, &QAction::triggered, this,
            &View2DWidget::saveTextureAsImage);

    // Add copy button
    QAction* copyAction = toolbar->addAction(QIcon(":/icons/copy.svg"),
                                             "Copy Texture to Clipboard");
    connect(copyAction, &QAction::triggered, this,
            &View2DWidget::copyTextureToClipboard);

    // Add tile toggle button
    QAction* tileAction =
        toolbar->addAction(QIcon(":/icons/grid.svg"), "Toggle 3x3 Tile View");
    tileAction->setCheckable(true);
    connect(tileAction, &QAction::triggered, this,
            &View2DWidget::toggleTileView);

    // Add recenter button
    QAction* recenterAction =
        toolbar->addAction(QIcon(":/icons/crosshair.svg"), "Recenter View");
    connect(recenterAction, &QAction::triggered, this,
            &View2DWidget::recenterView);

    graph = new View2DGraph(this);
    this->setCentralWidget(graph);
}

void View2DWidget::setSelectedNode(const TextureNodePtr& node)
{
    this->node = node;
    this->graph->setSelectedNode(node);
}

void View2DWidget::clearSelection() {}

void View2DWidget::reRenderNode()
{
    // this->graph->scene()->invalidate();
    this->graph->updatePreview();
}

void View2DWidget::setTextureRenderer(TextureRenderer* renderer)
{
    connect(renderer, &TextureRenderer::thumbnailGenerated,
            [=](const QString& nodeId, GLint texId, const QPixmap& pixmap) {
                if (!!node && node->id == nodeId) {
                    // this->graph->scene()->invalidate();
                    this->graph->updatePreview();
                }
            });
}

void View2DWidget::saveTextureAsImage()
{
    if (!node) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save Texture"), "", tr("PNG Images (*.png);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    // Ensure .png extension
    if (!fileName.endsWith(".png", Qt::CaseInsensitive)) {
        fileName += ".png";
    }

    // Get the texture ID
    GLuint texId = node->textureId();
    if (texId == 0) {
        return;
    }

    // Bind the texture and get its dimensions
    glBindTexture(GL_TEXTURE_2D, texId);
    GLint width, height;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    // Read texture data
    QImage image(width, height, QImage::Format_RGBA8888);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

    // Flip image vertically (OpenGL coordinates are bottom-up)
    image = image.mirrored(false, true);

    // Save the image
    image.save(fileName);
}

void View2DWidget::toggleTileView()
{
    showTiled = !showTiled;
    if (graph && graph->preview) {
        graph->preview->setTiled(showTiled);
        graph->preview->update();
    }
}

void View2DWidget::recenterView()
{
    if (graph && graph->preview) {
        // Reset transform
        graph->resetTransform();
        // Set initial scale
        graph->scale(0.3, 0.3);
        // Reset scene rect to center on origin
        QRectF previewRect = graph->preview->boundingRect();
        graph->setSceneRect(previewRect);
    }
}

void View2DWidget::copyTextureToClipboard()
{
    if (!node) {
        return;
    }

    // Get the texture ID
    GLuint texId = node->textureId();
    if (texId == 0) {
        return;
    }

    // Bind the texture and get its dimensions
    glBindTexture(GL_TEXTURE_2D, texId);
    GLint width, height;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    // Read texture data
    QImage image(width, height, QImage::Format_RGBA8888);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

    // Flip image vertically (OpenGL coordinates are bottom-up)
    image = image.mirrored(false, true);

    // Copy to clipboard
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setImage(image);

    // Show confirmation toast
    showToast("Texture copied to clipboard");
}

void View2DWidget::showToast(const QString& message, int duration)
{
    QLabel* toast = new QLabel(message, this);
    toast->setStyleSheet("QLabel {"
                         "  background-color: rgba(50, 50, 50, 200);"
                         "  color: white;"
                         "  padding: 10px 20px;"
                         "  border-radius: 5px;"
                         "}");
    toast->setAlignment(Qt::AlignCenter);
    toast->adjustSize();

    // Position at bottom center
    int x = (width() - toast->width()) / 2;
    int y = height() - toast->height() - 50;
    toast->move(x, y);

    // Fade in
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(toast);
    toast->setGraphicsEffect(effect);
    QPropertyAnimation* fadeIn = new QPropertyAnimation(effect, "opacity");
    fadeIn->setDuration(200);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    toast->show();
    toast->raise();

    // Fade out and delete
    QTimer::singleShot(duration, [toast]() {
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(toast);
        toast->setGraphicsEffect(effect);
        QPropertyAnimation* fadeOut = new QPropertyAnimation(effect, "opacity");
        fadeOut->setDuration(200);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
        QTimer::singleShot(200, toast, &QLabel::deleteLater);
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
    this->preview->update();
    // this->scene()->invalidate();
};

void View2DGraph::updatePreview() { this->preview->update(); }

void View2DGraph::clearSelection() {};

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

NodePreviewGraphicsItem::~NodePreviewGraphicsItem()
{
    cleanupGL();
}

void NodePreviewGraphicsItem::initializeGL()
{
    if (glInitialized) return;
    
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) return;
    
    QOpenGLFunctions* f = ctx->functions();
    
    // Create shader program
    shaderProgram = new QOpenGLShaderProgram();
    
    const char* vertexShaderSource = R"(
        #version 150
        in vec2 position;
        in vec2 texCoord;
        out vec2 vTexCoord;
        uniform mat4 projectionMatrix;
        void main() {
            gl_Position = projectionMatrix * vec4(position, 0.0, 1.0);
            vTexCoord = texCoord;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 150
        in vec2 vTexCoord;
        out vec4 fragColor;
        uniform sampler2D textureSampler;
        void main() {
            fragColor = texture(textureSampler, vTexCoord);
        }
    )";
    
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    shaderProgram->link();
    
    // Create VAO and VBO
    vao = new QOpenGLVertexArrayObject();
    vao->create();
    
    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->setUsagePattern(QOpenGLBuffer::DynamicDraw);
    
    glInitialized = true;
}

void NodePreviewGraphicsItem::cleanupGL()
{
    if (!glInitialized) return;
    
    delete shaderProgram;
    shaderProgram = nullptr;
    
    if (vbo) {
        vbo->destroy();
        delete vbo;
        vbo = nullptr;
    }
    
    if (vao) {
        vao->destroy();
        delete vao;
        vao = nullptr;
    }
    
    glInitialized = false;
}

QRectF NodePreviewGraphicsItem::boundingRect() const
{
    if (tiled) {
        // Center the 3x3 grid around origin
        return QRectF(-1500, -1500, 3000, 3000);
    }
    return QRectF(-500, -500, 1000, 1000);
}

void NodePreviewGraphicsItem::setNode(const TextureNodePtr& node)
{
    this->node = node;
    if (!!node)
        this->show();
}
void NodePreviewGraphicsItem::clearNode() { this->node.reset(); }

void NodePreviewGraphicsItem::setTiled(bool tiled)
{
    if (this->tiled != tiled) {
        prepareGeometryChange();
        this->tiled = tiled;
    }
}

void NodePreviewGraphicsItem::paint(QPainter* painter,
                                    QStyleOptionGraphicsItem const* option,
                                    QWidget* widget)
{
    auto type = painter->paintEngine()->type();
    if (type != QPaintEngine::OpenGL && type != QPaintEngine::OpenGL2) {
        qWarning() << "Paint engine needs to be OPENGL!";
        return;
    }

    if (!node) return;

    auto rect = boundingRect();
    
    painter->beginNativePainting();
    
    // Initialize OpenGL resources if needed
    initializeGL();
    
    if (!glInitialized || !shaderProgram || !vao || !vbo) {
        painter->endNativePainting();
        return;
    }
    
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    
    // Get the current viewport and create orthographic projection
    GLint viewport[4];
    f->glGetIntegerv(GL_VIEWPORT, viewport);
    
    // Create orthographic projection matrix matching the scene coordinates
    // The painter's transform converts scene coords to device coords
    QTransform transform = painter->combinedTransform();
    QMatrix4x4 projectionMatrix;
    projectionMatrix.ortho(0, viewport[2], viewport[3], 0, -1, 1);
    
    // Build vertex data
    QVector<GLfloat> vertices;
    
    auto addQuad = [&](float x0, float y0, float x1, float y1) {
        // Transform scene coordinates to device coordinates
        QPointF p0 = transform.map(QPointF(x0, y0));
        QPointF p1 = transform.map(QPointF(x1, y0));
        QPointF p2 = transform.map(QPointF(x1, y1));
        QPointF p3 = transform.map(QPointF(x0, y1));
        
        // Two triangles for a quad: position (x,y) + texcoord (u,v)
        // Triangle 1
        vertices << p0.x() << p0.y() << 0.0f << 1.0f;
        vertices << p1.x() << p1.y() << 1.0f << 1.0f;
        vertices << p2.x() << p2.y() << 1.0f << 0.0f;
        // Triangle 2
        vertices << p0.x() << p0.y() << 0.0f << 1.0f;
        vertices << p2.x() << p2.y() << 1.0f << 0.0f;
        vertices << p3.x() << p3.y() << 0.0f << 0.0f;
    };
    
    if (tiled) {
        float tileWidth = rect.width() / 3.0f;
        float tileHeight = rect.height() / 3.0f;
        
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                float x0 = rect.x() + x * tileWidth;
                float y0 = rect.y() + y * tileHeight;
                float x1 = rect.x() + (x + 1) * tileWidth;
                float y1 = rect.y() + (y + 1) * tileHeight;
                addQuad(x0, y0, x1, y1);
            }
        }
    } else {
        addQuad(rect.x(), rect.y(), rect.x() + rect.width(), rect.y() + rect.height());
    }
    
    // Setup state
    f->glDisable(GL_BLEND);
    f->glDisable(GL_DEPTH_TEST);
    
    // Bind shader
    shaderProgram->bind();
    shaderProgram->setUniformValue("projectionMatrix", projectionMatrix);
    shaderProgram->setUniformValue("textureSampler", 0);
    
    // Bind texture
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, node->textureId());
    
    // Setup VAO and VBO
    vao->bind();
    vbo->bind();
    vbo->allocate(vertices.constData(), vertices.size() * sizeof(GLfloat));
    
    // Setup vertex attributes
    int positionLoc = shaderProgram->attributeLocation("position");
    int texCoordLoc = shaderProgram->attributeLocation("texCoord");
    
    shaderProgram->enableAttributeArray(positionLoc);
    shaderProgram->enableAttributeArray(texCoordLoc);
    shaderProgram->setAttributeBuffer(positionLoc, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
    shaderProgram->setAttributeBuffer(texCoordLoc, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    
    // Draw
    f->glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 4);
    
    // Cleanup
    shaderProgram->disableAttributeArray(positionLoc);
    shaderProgram->disableAttributeArray(texCoordLoc);
    vbo->release();
    vao->release();
    shaderProgram->release();
    
    f->glEnable(GL_BLEND);
    
    painter->endNativePainting();
}