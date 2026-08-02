#include "texturerenderer.h"
#include "noderenderer.h"
#include "renderworker.h"
// #include "../models.h"

// https://forum.qt.io/topic/84779/how-to-create-a-qoffscreensurface-correctly/4
// #include <QGLFramebufferObject>
#include <QImage>
#include <QOffscreenSurface>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QTimer>
// Qt6 only!!
#include <QOpenGLVersionFunctionsFactory>
// #include <QOpenGLPaintDevice>
// #include <QtGui/QOpenGLFunctions_3_3_Core>

#include "../props.h"
#include "models.h"

// #define RENDER_IN_MAIN_THREAD

enum class VertexUsage : int {
    Position = 0,
    Color = 1,
    TexCoord0 = 2,
    TexCoord1 = 3,
    TexCoord2 = 4,
    TexCoord3 = 5,
    Normal = 6,
    Tangent = 7,
    Count = 8
};

const int TEXTURE_SIZE = 1024;

// https://github.com/cromop/mOffscreenRendering/blob/master/OGLWidget.cpp
// https://github.com/florianblume/Qt3D-OffscreenRenderer/blob/master/offscreensurfaceframegraph.h
// https://stackoverflow.com/questions/60515589/offscreen-render-with-qoffscreensurface-using-docker
void TextureRenderer::testRendering()
{
    // create shader
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex);
    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment);
    auto program = new QOpenGLShaderProgram;

    QString vSource = R""""(
        #version 150 core

        in vec3 a_pos;
        in vec2 a_texCoord;

        out vec2 v_texCoord;

        void main()
        {
                v_texCoord = a_texCoord*vec2(1,1);
                gl_Position = vec4(a_pos,1);
        }
    )"""";

    QString fSource = R""""(
        #version 150 core

        in vec2 v_texCoord;

        out vec4 fragColor;

        void main()
        {
            vec4 col = vec4(v_texCoord.x, v_texCoord.y, 1.0, 1.0);

            fragColor = col;
        }
    )"""";

    if (!vshader->compileSourceCode(vSource)) {
        qDebug() << "VERTEX SHADER ERROR";
        qDebug() << vshader->log();
    }

    if (!fshader->compileSourceCode(fSource)) {
        qDebug() << "FRAGMENT SHADER ERROR";
        qDebug() << fshader->log();
    }

    program->removeAllShaders();

    program->addShader(vshader);
    program->addShader(fshader);

    program->bindAttributeLocation("a_pos", (int)VertexUsage::Position);
    program->bindAttributeLocation("a_color", (int)VertexUsage::Color);
    program->bindAttributeLocation("a_texCoord", (int)VertexUsage::TexCoord0);

    if (!program->link()) {
        qDebug() << "SHADER LINK ERROR";
        qDebug() << program->log();
    }

    // render quad (bind vbo, shader, vbo, etc)
    fbo->bind();
    gl->glViewport(0, 0, TEXTURE_SIZE, TEXTURE_SIZE);

    gl->glClearColor(0, 0, 0, 1);
    gl->glClearDepth(0);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vao->bind();

    program->bind();

    vbo->bind();
    gl->glEnableVertexAttribArray((int)VertexUsage::Position);
    gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
    gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT, GL_FALSE,
                              5 * sizeof(float), nullptr);
    gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT,
                              GL_FALSE, 5 * sizeof(float),
                              reinterpret_cast<void*>(3 * sizeof(float)));

    gl->glDrawArrays(GL_TRIANGLES, 0, 6);

    vbo->release();

    gl->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());

    // grab pixels to pixmap
    auto img = fbo->toImage();
    img.save("./render.png");

    // save to desktop
}

void TextureRenderer::setup()
{
    // create surface
    surface = new QOffscreenSurface();
    // QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    // format.setMajorVersion(3);
    // format.setMinorVersion(2);

    QSurfaceFormat format;
    format.setDepthBufferSize(32);
    format.setMajorVersion(3);
    format.setMinorVersion(2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(1);
    format.setSwapInterval(0);
    format.setOption(QSurfaceFormat::DebugContext); // for debugging

    surface->setFormat(format);
    surface->create();

    // create context
    ctx = new QOpenGLContext();
    // https://doc.qt.io/qt-6/qopenglcontext.html#globalShareContext
    auto* globalContext = QOpenGLContext::globalShareContext();
    ctx->setShareContext(globalContext);
    ctx->setFormat(format);
    if (!ctx->create()) {
        qFatal("unable to create surface!");
    }

    ctx->makeCurrent(surface);

    // https://doc-snapshots.qt.io/qt6-dev/gui-changes-qt6.html
    gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_2_Core>(ctx);
    if (!gl) {
        qFatal("Could not obtain required OpenGL context version");
    }

    // setup debugging
    QOpenGLDebugLogger* logger = new QOpenGLDebugLogger();
    logger->initialize();

    QObject::connect(logger, &QOpenGLDebugLogger::messageLogged,
                     [=](const QOpenGLDebugMessage& debugMessage) {
                         qDebug() << debugMessage;
                     });

    logger->startLogging();

    gl->initializeOpenGLFunctions();

    // simple setup
    gl->glViewport(0, 0, TEXTURE_SIZE, TEXTURE_SIZE);
    gl->glDisable(GL_BLEND);
    gl->glDisable(GL_DEPTH_TEST);
    gl->glDisable(GL_CULL_FACE);

    // setup vbo
    vao = new QOpenGLVertexArrayObject;
    if (vao->create())
        vao->bind();

    // setup vertex array
    QVector<float> data;
    // TRIANGLE 1
    data.append(-1);
    data.append(-1);
    data.append(0);

    data.append(0);
    data.append(0);

    data.append(1);
    data.append(-1);
    data.append(0);

    data.append(1);
    data.append(0);

    data.append(-1);
    data.append(1);
    data.append(0);

    data.append(0);
    data.append(1);

    // TRIANGLE 2
    data.append(-1);
    data.append(1);
    data.append(0);

    data.append(0);
    data.append(1);

    data.append(1);
    data.append(-1);
    data.append(0);

    data.append(1);
    data.append(0);

    data.append(1);
    data.append(1);
    data.append(0);

    data.append(1);
    data.append(1);

    vbo = new QOpenGLBuffer;
    vbo->create();
    vbo->bind();
    vbo->allocate(data.constData(), data.count() * sizeof(float));
    vbo->release();

    // create fbo
    // https://doc.qt.io/qt-6/qopenglframebufferobject.html
    // https://www.qt.io/blog/2015/09/21/using-modern-opengl-es-features-with-qopenglframebufferobject-in-qt-5-6
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setInternalTextureFormat(GL_RGBA32F);
    fbo = new QOpenGLFramebufferObject(TEXTURE_SIZE, TEXTURE_SIZE, fboFormat);
    if (!fbo->isValid()) {
        qFatal("FBO could not be created");
    }

    this->initRenderWorker();
}

TextureRenderer::TextureRenderer() { this->setup(); }

TextureRenderer::~TextureRenderer()
{
    // Stop and clean up render worker thread
    if (renderWorker) {
        renderWorker->kill();
    }

#ifndef RENDER_IN_MAIN_THREAD
    if (renderThread) {
        renderThread->quit();
        renderThread->wait();
        delete renderThread;
        renderThread = nullptr;
    }
#endif

    if (renderWorker) {
        delete renderWorker;
        renderWorker = nullptr;
    }

    // Clean up OpenGL resources (must have context current)
    if (ctx && surface) {
        ctx->makeCurrent(surface);

        if (fbo) {
            delete fbo;
            fbo = nullptr;
        }

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

        ctx->doneCurrent();
    }

    // Clean up context and surface
    if (ctx) {
        delete ctx;
        ctx = nullptr;
    }

    if (surface) {
        surface->destroy();
        delete surface;
        surface = nullptr;
    }
}

void TextureRenderer::setProject(TextureProjectPtr project)
{
    this->project = project;
}

void TextureRenderer::update()
{
    if (!project)
        return;

    // check for nodes that need updating and update
    for (auto& node : project->nodes) {
        // Defensive: a null entry should never reach the map now that lookups
        // use .value() (Step 1), but guard the render loop regardless.
        if (!node)
            continue;

        if (!node->isGraphicsResourcesInitialized()) {
            // create texture
            initializeNodeGraphicsResources(node);
        }

        // if the resolution has changed, resize texture
        // Deferred while a render is in flight: the texture/FBO being
        // replaced here may be captured as an input GLuint in the command
        // currently queued/executing on the worker thread. Once that
        // command completes, nodeRendered() re-invokes update(), which will
        // pick this resize back up.
        if (!renderInFlight &&
            (project->textureWidth != node->textureWidth ||
             project->textureHeight != node->textureHeight)) {
            this->createNodeTexture(node);

            // clear pixmap and emit thumbnail changed?
        }
    }

    if (!renderInFlight)
        this->queueNextNodeToRender();
}

void TextureRenderer::updateOld()
{
    if (!project)
        return;

    // check for nodes that need updating and update
    for (auto& node : project->nodes) {
        if (!node->isGraphicsResourcesInitialized()) {
            // create texture
            initializeNodeGraphicsResources(node);
        }

        // if the resolution has changed, resize texture
        if (project->textureWidth != node->textureWidth ||
            project->textureHeight != node->textureHeight) {
            // resize
            // resizeNodeTexture(node);
            node->textureWidth = project->textureWidth;
            node->textureHeight = project->textureHeight;
            QOpenGLFramebufferObjectFormat fboFormat;
            fboFormat.setInternalTextureFormat(GL_RGBA32F);
            node->texture = new QOpenGLFramebufferObject(
                node->textureWidth, node->textureHeight, fboFormat);

            // clear pixmap and emit thumbnail changed?
        }

        // if (node->needsUpdate()) {

        //     // process
        // }
    }

    ctx->makeCurrent(surface);
    // todo: use quota
    while (true) {
        auto nextNode = getNextUpdatableNode();
        if (!nextNode)
            break;

        qDebug() << "Rendering node: " << nextNode->id;
        renderNode(nextNode);

        nextNode->isDirty = false;

        // auto img = nextNode->texture->toImage();
        // emit thumbnailGenerated(nextNode->id,  QPixmap::fromImage(img));

        auto texId = nextNode->texture->texture();
        emit thumbnailGenerated(nextNode->id, texId, QPixmap());
    }

    ctx->doneCurrent();
}

void TextureRenderer::initializeNodeGraphicsResources(
    const TextureNodePtr& node)
{
    this->createNodeTexture(node);

    ctx->makeCurrent(surface);
    // build and compile shaders
    node->shader = buildShaderForNode(node);
    ctx->doneCurrent();
}

void TextureRenderer::createNodeTexture(const TextureNodePtr& node)
{
    ctx->makeCurrent(surface);

    // Safe to free now: callers only reach here when !renderInFlight, so no
    // queued/executing RenderCommand can be holding this texture's GLuint
    // as an input.
    if (node->texture) {
        delete node->texture;
        node->texture = nullptr;
    }

    // create fbo
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setInternalTextureFormat(GL_RGBA32F);
    node->texture = new QOpenGLFramebufferObject(
        project->textureWidth, project->textureHeight, fboFormat);
    node->textureWidth = project->textureWidth;
    node->textureHeight = project->textureHeight;

    if (!node->texture->isValid()) {
        qFatal("FBO could not be created");
    }

    // make texture wrap
    gl->glBindTexture(GL_TEXTURE_2D, node->textureId());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gl->glBindTexture(GL_TEXTURE_2D, 0);

    // This texture ID is shared into the render worker's context on another
    // thread; flush so its creation is visible there before it's used.
    gl->glFlush();

    ctx->doneCurrent();
}

void TextureRenderer::renderNode(const TextureNodePtr& node)
{
    node->texture->bind();
    gl->glViewport(0, 0, node->textureWidth, node->textureHeight);

    gl->glClearColor(0, 0, 0, 1);
    gl->glClearDepth(0);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vao->bind();

    auto shader = node->shader;

    if (shader->isLinked()) {
        shader->bind();

        // clear all inputs
        int texIndex = 0;
        for (auto input : node->inputs) {
            gl->glActiveTexture(GL_TEXTURE0 + texIndex);
            gl->glBindTexture(GL_TEXTURE_2D, 0);

            // gl->glUniform1i(node->shader->uniformLocation(input), 0);
            shader->setUniformValue(input.toStdString().c_str(), 0);
            shader->setUniformValue(
                (input + "_connected").toStdString().c_str(), 0);

            texIndex++;
        }

        // pass inputs
        auto nodeInputs = getNodeInputs(node);
        texIndex = 0;
        for (auto nodeInput : nodeInputs) {
            gl->glActiveTexture(GL_TEXTURE0 + texIndex);
            // if (!nodeInput.node->texture->bind())
            //     qFatal("could not bind texture");
            gl->glBindTexture(GL_TEXTURE_2D,
                              nodeInput.node->texture->texture());

            auto name = nodeInput.name;
            // gl->glUniform1i(node->shader->uniformLocation(input), 0);
            shader->setUniformValue(name.toStdString().c_str(), texIndex);
            shader->setUniformValue((name + "_connected").toStdString().c_str(),
                                    1);

            texIndex++;
        }

        // pass seed
        shader->setUniformValue(
            "_seed", (GLfloat)(project->randomSeed + node->randomSeed));

        // texture size
        shader->setUniformValue(
            "_textureSize",
            QVector2D(project->textureWidth, project->textureHeight));

        // pass props
        for (auto prop : node->props) {
            auto propCString = ("prop_" + prop->name.toStdString());
            auto propName = propCString.c_str();
            // qDebug() << "glsl prop: " << propName;
            switch (prop->type) {
            case PropType::Int: {
                auto intVal = ((IntProp*)prop)->value;
                shader->setUniformValue(propName, (GLint)intVal);
            } break;
            case PropType::Float: {
                auto floatVal = ((FloatProp*)prop)->value;
                shader->setUniformValue(propName, (GLfloat)floatVal);
            } break;
            case PropType::Bool: {
                auto boolVal = ((BoolProp*)prop)->value;
                shader->setUniformValue(propName, (GLint)boolVal == true);
            } break;
            case PropType::Enum: {
                auto enumVal = ((EnumProp*)prop)->index;
                shader->setUniformValue(propName, (GLint)enumVal);
            } break;
            case PropType::Color: {
                auto colorVal = ((ColorProp*)prop)->value;
                shader->setUniformValue(
                    propName, QVector4D(colorVal.redF(), colorVal.greenF(),
                                        colorVal.blueF(), colorVal.alphaF()));
            } break;
            case PropType::Gradient:
                // todo: pass gradient
                break;
            }
        }

        // render triangles
        vbo->bind();
        gl->glEnableVertexAttribArray((int)VertexUsage::Position);
        gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
        gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT,
                                  GL_FALSE, 5 * sizeof(float), nullptr);
        gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT,
                                  GL_FALSE, 5 * sizeof(float),
                                  reinterpret_cast<void*>(3 * sizeof(float)));

        gl->glDrawArrays(GL_TRIANGLES, 0, 6);

        vbo->release();
    }

    vao->release();

    // gl->glBindFramebuffer(GL_FRAMEBUFFER,
    // ctx->defaultFramebufferObject());

    // grab pixels to pixmap
    // auto img = node->texture->toImage();
    // img.save(node->id + ".png");

    node->texture->release();
}

void TextureRenderer::initRenderWorker()
{
    renderWorker = new RenderWorker();
    renderWorker->initSurface();

    QObject::connect(renderWorker, &RenderWorker::nodeRendered, this,
                     &TextureRenderer::nodeRendered);

#ifdef RENDER_IN_MAIN_THREAD
    // ensure it creates its own context and resources on main thread
    renderWorker->setup();
#else
    renderThread = new QThread();

    renderWorker->moveToThread(renderThread);

    QObject::connect(renderThread, &QThread::started, renderWorker,
                     &RenderWorker::run);

    renderThread->start();
#endif
}

void TextureRenderer::nodeRendered(const QString& nodeId, GLuint texId)
{
    qDebug() << "TextureRenderer: Node rendered:" << nodeId;
    renderInFlight = false;
    emit thumbnailGenerated(nodeId, texId, QPixmap());
    // update() re-checks pending resizes deferred while a render was in
    // flight, then queues the next node.
    this->update();
    if (project) {
        int total = project->nodes.size();
        int clean = 0;
        for (const auto& n : project->nodes)
            if (!n->isDirty) clean++;
        emit renderProgress(clean, total);
    }
}

void TextureRenderer::queueNextNodeToRender()
{
    auto nextNode = getNextUpdatableNode();
    if (!!nextNode) {
        RenderCommand cmd;
        cmd.textureWidth = nextNode->textureWidth;
        cmd.textureHeight = nextNode->textureHeight;
        cmd.fboId = nextNode->texture->handle();
        cmd.textureId = nextNode->textureId();
        cmd.nodeId = nextNode->id;
        cmd.shaderId = nextNode->shader->programId();
        cmd.shaderLinked = nextNode->shader->isLinked();
        cmd.randomSeed = project->randomSeed + nextNode->randomSeed;

        // Custom renderer support
        cmd.renderer = nextNode->createRenderer();
        if (cmd.renderer) {
            cmd.renderData = nextNode->createRenderData();
        }

        // CPU processing support
        cmd.usesCpuProcessing = nextNode->usesCpuProcessing;
        // Keep the node alive for the lifetime of the command (it may still
        // be queued or mid-render on the worker thread if the node is
        // removed from the project in the meantime).
        cmd.nodePtr = nextNode;

        cmd.inputNames = nextNode->inputs;

        // inputs
        auto nodeInputs = getNodeInputs(nextNode);
        for (auto input : nodeInputs) {
            RenderNodeInput rin;
            rin.nodeId = input.node->id;
            rin.inputName = input.name;
            rin.textureId = input.node->textureId();
            cmd.inputs.append(rin);
        }

        // props
        for (auto prop : nextNode->props) {
            RenderProp rnp;
            rnp.propName = prop->name;
            rnp.propType = prop->type;
            rnp.textureId = 0;

            // Upload ImageProp textures to GPU on main thread
            if (prop->type == PropType::Image) {
                auto imageProp = (ImageProp*)prop;

                if (!imageProp->value.isNull()) {
                    ctx->makeCurrent(surface);
                    imageProp->updateTexture();

                    rnp.textureId = imageProp->getTextureId();

                    // Shared into the render worker's context on another
                    // thread; flush so the upload is visible there.
                    gl->glFlush();

                    ctx->doneCurrent();
                }
            }
            else {
                rnp.value = prop->getValue();
            }

            cmd.props.append(rnp);
        }

        QQueue<RenderCommand> queue;
        queue.enqueue(cmd);

        // pass to render worker to process
        renderWorker->setRenderQueue(queue);
        renderInFlight = true;

        // mark node as clean before rendering to avoid double-queuing
        nextNode->isDirty = false;

#ifdef RENDER_IN_MAIN_THREAD
        renderWorker->renderNextInQueue();
#endif
    }
}

QVector<NodeInput> TextureRenderer::getNodeInputs(const TextureNodePtr& node)
{
    QVector<NodeInput> inputs;
    for (auto con : project->connections) {
        if (con->rightNode == node) {
            inputs.append({con->leftNode, con->rightNodeInputName});
        }
    }

    return inputs;
}

TextureNodePtr TextureRenderer::getNextUpdatableNode() const
{
    // for each node, if node is dirty and all deps are
    // non-dirty the this is a valid node

    for (auto node : project->nodes) {
        if (!node)
            continue;

        if (!node->isDirty)
            continue;

        auto hasCleanDeps = true;

        // we have a dirty node, check if all deps are clean
        auto deps = project->getNodeDependencies(node->id);
        for (auto dep : deps) {
            // A null dep means an input connection references a node that no
            // longer exists; treat it as not-yet-renderable rather than
            // dereferencing it.
            if (!dep || dep->isDirty) {
                hasCleanDeps = false;
                break;
            }
        }

        if (hasCleanDeps) {
            return node;
        }
    }

    return TextureNodePtr(nullptr);
}

QOpenGLShaderProgram*
TextureRenderer::buildShaderForNode(const TextureNodePtr& node)
{
    ctx->makeCurrent(surface);

    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex);
    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment);
    auto program = new QOpenGLShaderProgram;

    // Build input/prop declaration lists for RenderResourceCache helpers
    QStringList inputNames = node->inputs;

    QList<QPair<QString, int>> propTypes;
    for (auto prop : node->props)
        propTypes.append({prop->name, (int)prop->type});

    QString fSource = RenderResourceCache::fragmentPreamble()
                    + RenderResourceCache::randomLib()
                    + RenderResourceCache::gradientLib()
                    + RenderResourceCache::curveLib()
                    + RenderResourceCache::generateInputDeclarations(inputNames)
                    + RenderResourceCache::generatePropDeclarations(propTypes)
                    + "#line 0\n"
                    + node->shaderSource;

    if (!vshader->compileSourceCode(RenderResourceCache::standardVertexSource())) {
        qDebug() << "VERTEX SHADER ERROR";
        qDebug() << vshader->log();
    }

    if (!fshader->compileSourceCode(fSource)) {
        qDebug() << "FRAGMENT SHADER ERROR";
        qDebug() << fshader->log();
    }

    program->removeAllShaders();
    program->addShader(vshader);
    program->addShader(fshader);

    program->bindAttributeLocation("a_pos", (int)VertexUsage::Position);
    program->bindAttributeLocation("a_color", (int)VertexUsage::Color);
    program->bindAttributeLocation("a_texCoord", (int)VertexUsage::TexCoord0);

    if (!program->link()) {
        qDebug() << "SHADER LINK ERROR";
        qDebug() << program->log();
    }

    // Shared into the render worker's context on another thread; flush so
    // the link is visible there before glUseProgram() is called on it.
    gl->glFlush();

    ctx->doneCurrent();

    return program;
}