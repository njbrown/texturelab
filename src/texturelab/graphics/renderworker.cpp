#include "renderworker.h"
#include "texturerenderer.h"
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
// Qt6 only!!
#include <QOpenGLVersionFunctionsFactory>
#include <renderer/renderer.h>

// https://renderdoc.org/docs/in_application_api.html
#include "renderdoc_app.h"
#ifdef __linux__
#include <dlfcn.h>
#endif

const int TEXTURE_SIZE = 1024;

RENDERDOC_API_1_1_2* rdoc_api = NULL;

RenderWorker::RenderWorker()
    : QObject(), surface(nullptr), ctx(nullptr), gl(nullptr), vao(nullptr),
      vbo(nullptr), vshader(nullptr), fshader(nullptr), fbo(nullptr),
      running(false)
{
}

void RenderWorker::setRenderQueue(QQueue<RenderCommand> renderQueue)
{
    QMutexLocker locker(&mutex);
    this->renderQueue = renderQueue;
}

void RenderWorker::run()
{
    this->setup();

    while (running) {
        this->renderNextInQueue();
    }
}

void RenderWorker::renderNextInQueue()
{
    mutex.lock();
    if (!renderQueue.isEmpty()) {
        RenderCommand command = renderQueue.dequeue();
        mutex.unlock();
        this->processRenderCommand(command);
    }
    else {
        mutex.unlock();
    }
}

void RenderWorker::setup()
{
    running = true;
    // Initialize OpenGL context or other necessary setups here
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
    format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
    format.setOption(QSurfaceFormat::DebugContext); // for debugging

    surface->setFormat(format);
    surface->create();

    // create context
    ctx = new QOpenGLContext();
    // https://doc.qt.io/qt-6/qopenglcontext.html#globalShareContext
    ctx->setShareContext(QOpenGLContext::globalShareContext());
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
                         if (debugMessage.type() ==
                             QOpenGLDebugMessage::ErrorType)
                             qFatal() << debugMessage;
                         //  else
                         //      qDebug() << debugMessage;
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
    fbo = new QOpenGLFramebufferObject(TEXTURE_SIZE, TEXTURE_SIZE);
    if (!fbo->isValid()) {
        qFatal("FBO could not be created");
    }

    // create FBO with no color attachment for depth-only rendering
    fboId = 0;
    gl->glGenFramebuffers(1, &fboId);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    // gl->glDrawBuffer(GL_NONE);
    // gl->glReadBuffer(GL_NONE);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);

#ifdef __linux__
    // setup renderdoc
    if (void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD)) {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI =
            (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
        RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&rdoc_api);
    }
#endif

    ctx->doneCurrent();
}

void RenderWorker::processRenderCommand(const RenderCommand& command)
{
    // Here you would bind the shader, set up inputs and props, and render to a
    // texture. This is a placeholder implementation.

    if (rdoc_api)
        rdoc_api->StartFrameCapture(NULL, NULL);

    ctx->makeCurrent(surface);

    // Simulate rendering process
    GLuint renderedTextureId =
        0; // Replace with actual texture ID after rendering

    // qDebug() << "RenderWorker: Processing render command for node:"
    //          << command.nodeId;

    gl->glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, command.textureId, 0);

    GLenum status = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        qFatal("FRAMEBUFFER IS NOT COMPLETE!");
        // qWarning("%s Framebuffer is not complete!", command.nodeId);
    }
    // fbo->bind();

    gl->glViewport(0, 0, command.textureWidth, command.textureHeight);

    gl->glClearColor(0, 1, 0, 1);
    gl->glClearDepth(0);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // qDebug() << "RenderWorker: Cleared framebuffer for node:" <<
    // command.nodeId;
    vao->bind();

    if (command.shaderLinked) {
        gl->glUseProgram(command.shaderId);

        // clear all inputs
        int texIndex = 0;
        for (auto input : command.inputs) {
            gl->glActiveTexture(GL_TEXTURE0 + texIndex);
            gl->glBindTexture(GL_TEXTURE_2D, 0);

            // gl->glUniform1i(node->shader->uniformLocation(input), 0);
            gl->glUniform1i(
                gl->glGetUniformLocation(command.shaderId,
                                         input.inputName.toStdString().c_str()),
                0);
            std::string connectedName =
                input.inputName.toStdString() + "_connected";
            gl->glUniform1i(gl->glGetUniformLocation(command.shaderId,
                                                     connectedName.c_str()),
                            0);

            texIndex++;
        }

        // pass inputs
        texIndex = 0;
        for (auto nodeInput : command.inputs) {
            gl->glActiveTexture(GL_TEXTURE0 + texIndex);
            // if (!nodeInput.node->texture->bind())
            //     qFatal("could not bind texture");
            gl->glBindTexture(GL_TEXTURE_2D, nodeInput.textureId);

            auto name = nodeInput.inputName;
            // gl->glUniform1i(node->shader->uniformLocation(input), 0);
            gl->glUniform1i(gl->glGetUniformLocation(
                                command.shaderId, name.toStdString().c_str()),
                            texIndex);
            std::string connectedName = name.toStdString() + "_connected";
            gl->glUniform1i(gl->glGetUniformLocation(command.shaderId,
                                                     connectedName.c_str()),
                            1);
            // shader->setUniformValue(name.toStdString().c_str(), texIndex);
            // shader->setUniformValue((name +
            // "_connected").toStdString().c_str(),
            //                         1);

            texIndex++;
        }

        // pass seed
        // shader->setUniformValue("_seed", (GLfloat)(command.randomSeed));
        gl->glUniform1f(gl->glGetUniformLocation(command.shaderId, "_seed"),
                        (GLfloat)(command.randomSeed));

        // texture size
        // shader->setUniformValue(
        //     "_textureSize",
        //     QVector2D(command.textureWidth, command.textureHeight));
        gl->glUniform2f(
            gl->glGetUniformLocation(command.shaderId, "_textureSize"),
            (GLfloat)(command.textureWidth), (GLfloat)(command.textureHeight));

        // pass props
        for (auto prop : command.props) {
            auto propCString = ("prop_" + prop.propName.toStdString());
            auto propName = propCString.c_str();
            // qDebug() << "glsl prop: " << propName;

            switch (prop.propType) {
            case PropType::Int: {
                GLint intVal = prop.value.toInt();
                gl->glUniform1i(
                    gl->glGetUniformLocation(command.shaderId, propName),
                    intVal);
            } break;
            case PropType::Float: {
                GLfloat floatVal = prop.value.toFloat();
                gl->glUniform1f(
                    gl->glGetUniformLocation(command.shaderId, propName),
                    floatVal);
            } break;
            case PropType::Bool: {
                GLint boolVal = prop.value.toBool() ? 1 : 0;
                gl->glUniform1i(
                    gl->glGetUniformLocation(command.shaderId, propName),
                    boolVal);
            } break;
            case PropType::Enum: {
                GLint enumVal = prop.value.toInt();
                gl->glUniform1i(
                    gl->glGetUniformLocation(command.shaderId, propName),
                    enumVal);
            } break;
            case PropType::Color: {
                auto colorVal = prop.value.value<QColor>();
                gl->glUniform4f(
                    gl->glGetUniformLocation(command.shaderId, propName),
                    colorVal.redF(), colorVal.greenF(), colorVal.blueF(),
                    colorVal.alphaF());
            } break;
            case PropType::Gradient:
                // todo: pass gradient
                break;
            case PropType::Image: {
                // Use pre-uploaded texture ID from main thread
                if (prop.textureId != 0) {
                    gl->glActiveTexture(GL_TEXTURE0 + texIndex);
                    gl->glBindTexture(GL_TEXTURE_2D, prop.textureId);
                    gl->glUniform1i(
                        gl->glGetUniformLocation(command.shaderId, propName),
                        texIndex);
                    texIndex++;
                }
                else {
                    // No texture provided, bind a default texture (e.g., white)
                    gl->glActiveTexture(GL_TEXTURE0 + texIndex);
                    gl->glBindTexture(GL_TEXTURE_2D, 0); // Bind to 0 or a
                                                         // default texture
                    gl->glUniform1i(
                        gl->glGetUniformLocation(command.shaderId, propName),
                        texIndex);
                    texIndex++;
                }
            } break;
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

    // gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // fbo->release();

    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, 0, 0);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());

    ctx->doneCurrent();

    if (rdoc_api)
        rdoc_api->EndFrameCapture(NULL, NULL);
    // Emit signal that node has been rendered
    emit nodeRendered(command.nodeId, command.textureId);
}

void RenderWorker::kill() { running = false; }
