#include "scene.h"
#include "comment.h"
#include "frame.h"
#include <QGraphicsDropShadowEffect>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QPaintEngine>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QUuid>

namespace nodegraph {

// Static member definitions for Node's OpenGL resources
QOpenGLShaderProgram* Node::shaderProgram = nullptr;
QOpenGLBuffer* Node::vbo = nullptr;
QOpenGLVertexArrayObject* Node::vao = nullptr;
bool Node::glInitialized = false;

void Node::initializeGL()
{
    if (glInitialized) return;
    
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) return;
    
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
            // 8px checkerboard in screen space
            vec2 tile = floor(gl_FragCoord.xy / 8.0);
            float checker = mod(tile.x + tile.y, 2.0);
            vec3 bg = mix(vec3(0.753), vec3(0.502), checker);

            vec4 texColor = texture(textureSampler, vTexCoord);
            fragColor = vec4(mix(bg, texColor.rgb, texColor.a), 1.0);
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

void Node::cleanupGL()
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

Scene::Scene() : QGraphicsScene()
{
    this->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

ScenePtr Scene::create() { return ScenePtr(new Scene()); }

void Scene::addNode(NodePtr node)
{
    this->addItem(node.data());
    nodes[node->id()] = node;
}

ConnectionPtr Scene::connectNodes(NodePtr leftNode, QString leftOutputName,
                                  NodePtr rightNode, QString rightInputName)
{
    auto leftPort = leftNode->getOutPortByName(leftOutputName);
    qDebug() << rightNode->getInPorts();
    auto rightPort = rightNode->getInPortByName(rightInputName);

    // create new connection item from ports
    auto conn = new Connection();
    conn->startPort = leftPort;
    conn->endPort = rightPort;
    conn->updatePosFromPorts();
    conn->updatePathFromPositions();

    ConnectionPtr connPtr(conn);

    // also add them to the ports
    leftPort->addConnection(connPtr);
    rightPort->addConnection(connPtr);

    this->addItem(conn);

    return connPtr;
}

NodePtr Scene::getNodeById(QString id) { return nodes[id]; }

void Scene::addFrame(FramePtr frame)
{
    this->addItem(frame.data());
    frames[frame->id()] = frame;
}

FramePtr Scene::getFrameById(QString id) { return frames[id]; }

void Scene::removeFrame(FramePtr frame)
{
    frame->hide();
    this->removeItem(frame.data());
    frames.remove(frame->id());
    frame->show();
}

void Scene::addComment(CommentPtr comment)
{
    this->addItem(comment.data());
    comments[comment->id()] = comment;
}

CommentPtr Scene::getCommentById(QString id) { return comments[id]; }

void Scene::removeComment(CommentPtr comment)
{
    comment->hide();
    this->removeItem(comment.data());
    comments.remove(comment->id());
    comment->show();
}

void Scene::removeNode(NodePtr node)
{
    // gather connections
    QList<ConnectionPtr> cons;
    for (auto port : node->getInPorts()) {
        cons.append(port->connections);
    }

    for (auto port : node->getOutPorts()) {
        cons.append(port->connections);
    }

    // remove connections
    for (auto con : cons) {
        this->removeConnection(con);
    }

    // remove node
    node->hide(); // fix display cache issue
    this->removeItem(node.data());

    // reshow here in case i forget when re-adding node for
    // undo-redo
    node->show();
}

void Scene::removeConnection(ConnectionPtr con)
{
    con->startPort->removeConnection(con);
    con->endPort->removeConnection(con);

    this->removeItem(con.data());
}

Scene::~Scene()
{
    // remove all items manually otherwise
    // smart point destructor of some items
    // will cause segfault when cleaning up
    auto items = this->items();
    for (auto item : items)
        this->removeItem(item);
}

Node::Node()
{
    // Generate unique ID for this node
    _id = QUuid::createUuid().toString();

    width = NODE_WIDTH;
    height = NODE_HEIGHT;
    isHovered = false;

    defaultBorderColor = QColor(0, 0, 0);
    highlightBorderColor = QColor(0, 0, 0);
    // highlightBorderColor = QColor(120, 120, 120);
    selectedBorderColor = QColor(200, 200, 200);

    setCacheMode(QGraphicsItem::NoCache);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

    setCursor(Qt::ClosedHandCursor);

    text = new QGraphicsTextItem(this);
    text->setFlag(QGraphicsItem::ItemIsFocusable, false);
    text->setFlag(QGraphicsItem::ItemIsSelectable, false);

    text->setPos(0, 0);
    text->setTextWidth(100);
    text->setDefaultTextColor(QColor(255, 255, 255));
    text->setZValue(5);

    // center title
    setName("Title");

    text->document()->setDocumentMargin(2);

    QFont font = text->font();
    font.setWeight(QFont::Bold);
    font.setPixelSize(12);
    text->setFont(font);

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect;
    effect->setBlurRadius(20);
    effect->setXOffset(0);
    effect->setYOffset(0);
    effect->setColor(QColor(00, 00, 00, 70));
    // setGraphicsEffect(effect); // forces node to raster remder
    // maybe render to node behind this to get same effect

    setAcceptHoverEvents(true);
    // setAcceptDrops(true);
}

NodePtr Node::create() { return NodePtr(new Node()); }

void Node::setCenter(float x, float y)
{
    setPos(x - NODE_WIDTH / 2.0f, y - NODE_HEIGHT / 2.0f);
}

QPointF Node::getCenter() const
{
    return QPointF(pos().x() + NODE_WIDTH / 2.0f, pos().y() + NODE_HEIGHT / 2.0f);
}

void Node::setName(QString name)
{
    this->name = name;
    text->setPlainText(name);

    QTextBlockFormat format;
    format.setAlignment(Qt::AlignCenter);
    QTextCursor cursor = text->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(format);
    cursor.clearSelection();
    text->setTextCursor(cursor);
}

void Node::setThumbnail(const QPixmap& pixmap)
{
    this->thumbnail = pixmap;
    this->update();
}

const QVector<PortPtr> Node::getInPorts() const { return inPorts; }

const QVector<PortPtr> Node::getOutPorts() const { return outPorts; }

void Node::addInPort(QString name)
{
    PortPtr port(new Port(this));
    port->name = name;
    port->portType = PortType::In;
    port->node = this->sharedFromThis();
    inPorts.append(port);

    // top and bottom padding for sockets
    const int pad = inPorts.count() < 5 ? 10 : 0;

    // sort in sockets
    int incr = (this->height - pad * 2) / inPorts.count();
    int mid = incr / 2.0;
    int i = 0;
    for (auto port : inPorts) {
        int y = pad + i * incr + mid;
        int x = 0;
        port->setCenter(x, y);
        i++;
    }
}

void Node::addOutPort(QString name)
{
    PortPtr port(new Port(this));
    port->name = name;
    port->portType = PortType::Out;
    port->node = this->sharedFromThis();
    outPorts.append(port);

    // top and bottom padding for sockets
    const int pad = outPorts.count() < 5 ? 10 : 0;

    // sort in sockets
    int incr = (this->height - pad * 2) / outPorts.count();
    int mid = incr / 2.0;
    int i = 0;
    for (auto port : outPorts) {
        int y = pad + i * incr + mid;
        int x = width;
        port->setCenter(x, y);
        i++;
    }
}

PortPtr Node::getPortById(QString id)
{
    for (auto port : inPorts) {
        if (port->id() == id)
            return port;
    }

    for (auto port : outPorts) {
        if (port->id() == id)
            return port;
    }

    Q_ASSERT(false);
}

PortPtr Node::getInPortByName(QString name)
{
    for (auto port : inPorts) {
        if (port->name == name)
            return port;
    }

    Q_ASSERT(false);
}

PortPtr Node::getOutPortByName(QString name)
{
    for (auto port : outPorts) {
        if (port->name == name)
            return port;
    }

    Q_ASSERT(false);
}

QRectF Node::boundingRect() const { return QRectF(0, 0, 100, 100); }

// void Node::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
// {
//     QGraphicsObject::mouseMoveEvent(event);
// }

void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    // if (event->button() == Qt::LeftButton) {
    //     emit selected(sharedFromThis());
    // }

    QGraphicsObject::mouseReleaseEvent(event);
}

void Node::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    this->text->hide();
    this->isHovered = true;
    QGraphicsObject::hoverEnterEvent(event);
}

void Node::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    this->text->show();
    this->isHovered = false;
    QGraphicsObject::hoverLeaveEvent(event);
}

void Node::paint(QPainter* painter, QStyleOptionGraphicsItem const* option,
                 QWidget* widget)
{
    const int titleHeight = 20;
    const int nodeWidth = width;
    const int nodeHeight = height;
    const int titleRadius = 4;
    const QColor titleColor(0, 0, 0);

    // // https://doc.qt.io/qt-5/qpainter.html#beginNativePainting
    // https://github.com/liff-engineer/WeeklyARTS/blob/d8605aa3bfb2641d2a13621262024a1edff7b661/2018_9_4/Mixin2D%263DinQt.md
    auto type = painter->paintEngine()->type();
    if (type != QPaintEngine::OpenGL && type != QPaintEngine::OpenGL2) {
        qWarning() << "Paint engine needs to be OPENGL!";
        // return;
    }

    auto rect = boundingRect();

    QColor borderColor;
    if (isSelected())
        borderColor = this->selectedBorderColor;
    else if (isHovered)
        borderColor = this->highlightBorderColor;
    else
        borderColor = this->defaultBorderColor;

    // not really needed
    // painter->setClipRect(option->exposedRect);

    // smooth rendering
    // painter->setRenderHint(QPainter::Antialiasing);
    // painter->setRenderHint(QPainter::TextAntialiasing);

    // title tab
    // QPainterPath titlePath;
    // titlePath.setFillRule(Qt::WindingFill);
    // titlePath.addRect(0, 10, width, titleHeight - 10);
    // titlePath.addRoundedRect(0, 0, nodeWidth, titleHeight, titleRadius,
    // titleRadius); painter->fillPath(titlePath, QBrush(QColor(255, 255,
    // 255)));

    // // draw text node seperator
    // QPainterPath block;
    // block.setFillRule(Qt::WindingFill);
    // block.addRect(0, titleHeight, nodeWidth, 3);
    // painter->fillPath(block, QBrush(QColor(30, 30, 30, 160)));

    // QPen pen(QColor(00, 00, 00, 250), .5);
    // QPen pen(borderColor, .5);
    // painter->setPen(pen);

    // background
    QPainterPath bgPath;
    bgPath.setFillRule(Qt::WindingFill);
    bgPath.addRoundedRect(0, 0, nodeWidth, nodeHeight, titleRadius,
                          titleRadius);
    painter->fillPath(bgPath, QBrush(QColor(10, 10, 10, 255)));

    if (!thumbnail.isNull()) {
        // Checkerboard background for alpha-transparent thumbnails
        static QPixmap checkerTile;
        if (checkerTile.isNull()) {
            checkerTile = QPixmap(16, 16);
            checkerTile.fill(QColor(0xC0, 0xC0, 0xC0));
            QPainter cp(&checkerTile);
            cp.fillRect(0, 0, 8, 8, QColor(0x80, 0x80, 0x80));
            cp.fillRect(8, 8, 8, 8, QColor(0x80, 0x80, 0x80));
        }
        painter->fillRect(QRect(0, 0, nodeWidth, nodeHeight), QBrush(checkerTile));
        painter->drawPixmap(QRect(0, 0, nodeWidth, nodeHeight), thumbnail);
    }

    // thumbnail as texture
    if (texId != 0) {
        // // https://doc.qt.io/qt-5/qpainter.html#beginNativePainting
        painter->beginNativePainting();

        // Initialize OpenGL resources if needed
        initializeGL();
        
        if (glInitialized && shaderProgram && vao && vbo) {
            QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
            
            // Get the current viewport and create orthographic projection
            GLint viewport[4];
            f->glGetIntegerv(GL_VIEWPORT, viewport);
            
            // Create orthographic projection matrix
            QTransform transform = painter->combinedTransform();
            QMatrix4x4 projectionMatrix;
            projectionMatrix.ortho(0, viewport[2], viewport[3], 0, -1, 1);
            
            // Build vertex data - transform scene coordinates to device coordinates
            QPointF p0 = transform.map(QPointF(0, 0));
            QPointF p1 = transform.map(QPointF(100, 0));
            QPointF p2 = transform.map(QPointF(100, 100));
            QPointF p3 = transform.map(QPointF(0, 100));
            
            // Two triangles for a quad: position (x,y) + texcoord (u,v)
            GLfloat vertices[] = {
                // Triangle 1
                (GLfloat)p0.x(), (GLfloat)p0.y(), 0.0f, 1.0f,
                (GLfloat)p1.x(), (GLfloat)p1.y(), 1.0f, 1.0f,
                (GLfloat)p2.x(), (GLfloat)p2.y(), 1.0f, 0.0f,
                // Triangle 2
                (GLfloat)p0.x(), (GLfloat)p0.y(), 0.0f, 1.0f,
                (GLfloat)p2.x(), (GLfloat)p2.y(), 1.0f, 0.0f,
                (GLfloat)p3.x(), (GLfloat)p3.y(), 0.0f, 0.0f,
            };
            
            // Setup state
            f->glDisable(GL_BLEND);
            f->glDisable(GL_DEPTH_TEST);
            
            // Bind shader
            shaderProgram->bind();
            shaderProgram->setUniformValue("projectionMatrix", projectionMatrix);
            shaderProgram->setUniformValue("textureSampler", 0);
            
            // Bind texture
            f->glActiveTexture(GL_TEXTURE0);
            f->glBindTexture(GL_TEXTURE_2D, texId);
            
            // Setup VAO and VBO
            vao->bind();
            vbo->bind();
            vbo->allocate(vertices, sizeof(vertices));
            
            // Setup vertex attributes
            int positionLoc = shaderProgram->attributeLocation("position");
            int texCoordLoc = shaderProgram->attributeLocation("texCoord");
            
            shaderProgram->enableAttributeArray(positionLoc);
            shaderProgram->enableAttributeArray(texCoordLoc);
            shaderProgram->setAttributeBuffer(positionLoc, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
            shaderProgram->setAttributeBuffer(texCoordLoc, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
            
            // Draw
            f->glDrawArrays(GL_TRIANGLES, 0, 6);
            
            // Cleanup
            shaderProgram->disableAttributeArray(positionLoc);
            shaderProgram->disableAttributeArray(texCoordLoc);
            vbo->release();
            vao->release();
            shaderProgram->release();
            
            f->glEnable(GL_BLEND);
        }

        painter->endNativePainting();
    }

    // draw highlight

    // top bar for text
    if (!isHovered) {
        QPainterPath bgPath;
        bgPath.setFillRule(Qt::WindingFill);
        bgPath.addRoundedRect(0, 0, nodeWidth, 18, titleRadius, titleRadius);
        painter->fillPath(bgPath, QBrush(QColor(0, 0, 0, 255)));

        text->paint(painter, option, widget);
    }

    // draw border
    painter->setPen(QPen(borderColor, 3));
    painter->drawRoundedRect(rect, titleRadius, titleRadius);
}

Node::~Node()
{
    // remove ownership from scene else scene will try to
    // clean it up after it's been deleted
    if (this->scene())
        this->scene()->removeItem(this);
}

QString Port::id() const { return _id; }

Port::Port(QGraphicsObject* parent) : QGraphicsObject(parent)
{
    setCursor(Qt::ClosedHandCursor);

    // this->setFlag(QGraphicsItem::ItemIsSelectable, false);
    this->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

    _radius = 7;
    name = "";
    portType = PortType::In;
    _id = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QRectF Port::boundingRect() const
{
    // return QRectF(-_radius, -_radius, _radius * 2, _radius * 2);

    // add extra space for hit testing
    return QRectF(-_radius * 2, -_radius * 2, _radius * 4, _radius * 4);
}

QRectF Port::actualRect() const
{
    return QRectF(-_radius, -_radius, _radius * 2, _radius * 2);
}

void Port::setCenter(float x, float y)
{
    // auto rect = this->boundingRect();
    // setPos(QPointF(x - rect.x() / 2, y - rect.y() / 2));
    setPos(QPointF(x, y));
}

QVariant Port::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemScenePositionHasChanged) {
        for (auto con : connections) {
            con->updatePosFromPorts();
            con->updatePathFromPositions();
        }
    }
    return value;
}

void Port::removeConnection(ConnectionPtr con)
{
    // todo: make sure this does what it's supposed to do
    connections.removeOne(con);
}

void Port::paint(QPainter* painter, QStyleOptionGraphicsItem const* option,
                 QWidget* widget)
{
    auto rect = actualRect();

    QPen pen(QColor(00, 00, 00, 250), 1.0f);
    painter->setPen(pen);

    // background
    QPainterPath bgPath;
    bgPath.setFillRule(Qt::WindingFill);
    // bgPath.addRoundedRect(-_radius, _radius, rect.width(), rect.height(),
    // rect.width() / 2, rect.height() / 2);
    bgPath.addRoundedRect(rect, _radius, _radius);
    painter->fillPath(bgPath, QBrush(QColor(170, 170, 170, 255)));

    // draw border
    painter->setPen(QPen(QColor(0, 0, 0), 3));
    painter->drawRoundedRect(rect, rect.width() / 2, rect.height() / 2);
}

Port::~Port()
{
    // remove ownership from scene else scene will try to
    // clean it up after it's been deleted
    if (this->scene())
        this->scene()->removeItem(this);
}

Connection::Connection()
{
    pos1 = QPointF(0, 0);
    pos2 = QPointF(0, 0);

    connectState = ConnectionState::Complete;

    auto pen = QPen(QColor(200, 200, 200));
    pen.setBrush(QColor(50, 150, 250));
    pen.setCapStyle(Qt::RoundCap);
    pen.setWidth(lineThickness);
    setPen(pen);
}

void Connection::updatePosFromPorts()
{
    pos1 = startPort->scenePos();
    pos2 = endPort->scenePos();
}

void Connection::updatePathFromPositions()
{
    p = new QPainterPath;
    p->moveTo(pos1);

    qreal dx = pos2.x() - pos1.x();
    qreal dy = pos2.y() - pos1.y();

    QPointF ctr1(pos1.x() + dx * 0.5, pos1.y());
    QPointF ctr2(pos2.x() - dx * 0.5, pos2.y());

    p->cubicTo(ctr1, ctr2, pos2);
    p->setFillRule(Qt::OddEvenFill);

    setPath(*p);
}

void Connection::paint(QPainter* painter,
                       const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->save();

    if (connectState == ConnectionState::Dragging) {
        QPen pen(QColor(150, 150, 150), lineThickness);
        pen.setStyle(Qt::DashLine);
        pen.setDashOffset(4);
        painter->setPen(pen);
        painter->drawPath(*p);

        painter->setPen(QPen(QColor(0, 0, 0), 3));
        painter->setBrush(QBrush(QColor(150, 150, 150)));
        painter->drawEllipse(pos1, 7, 7);

        painter->setPen(Qt::NoPen);
        painter->drawEllipse(pos2, 6, 6);
    }
    if (connectState == ConnectionState::Complete) {
        // create gradient for line
        QPen pen(QColor(170, 170, 170), lineThickness);
        painter->setPen(pen);
        painter->drawPath(*p);

        painter->setPen(QPen(QColor(0, 0, 0), 3));
        painter->setBrush(QBrush(QColor(170, 170, 170)));
        painter->drawEllipse(pos1, 7, 7);
        painter->drawEllipse(pos2, 7, 7);
    }

    painter->restore();

    Q_UNUSED(option);
    Q_UNUSED(widget);
}

Connection::~Connection()
{
    // remove ownership from scene else scene will try to
    // clean it up after it's been deleted
    if (this->scene())
        this->scene()->removeItem(this);
}

} // namespace nodegraph