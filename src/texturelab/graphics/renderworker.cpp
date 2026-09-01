#include "renderworker.h"
#include "../models.h"
#include "../systeminfo.h"
#include "../telemetry.h"
#include "../curve.h"
#include "gradient.h"
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

RENDERDOC_API_1_1_2* rdoc_api = nullptr;

namespace {

// Attach GL/VRAM state to a Sentry event before a qFatal takes the process
// down. Continuing past a broken framebuffer on the render thread isn't safe,
// but these used to arrive as bare aborts with an unreadable driver stack.
void reportFatalGlState(const char* what, Telemetry::Fields extra)
{
    const SystemInfo::GpuMemory mem = SystemInfo::queryGpuMemory();
    extra.emplace_back("vram_known", mem.known);
    extra.emplace_back("vram_total_mb",
                       mem.known ? mem.totalKb / 1024 : (int64_t)-1);
    extra.emplace_back("vram_available_mb",
                       mem.known ? mem.availableKb / 1024 : (int64_t)-1);
    extra.emplace_back("thread", std::string("render worker"));
    Telemetry::captureException(what, extra);
}

} // namespace

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

// Must be called from main/GUI thread before run() - Windows requires surface creation on GUI thread
void RenderWorker::initSurface()
{
    QSurfaceFormat format;
    format.setDepthBufferSize(32);
    format.setMajorVersion(3);
    format.setMinorVersion(2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(1);
    format.setSwapInterval(0);
    format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
    format.setOption(QSurfaceFormat::DebugContext); // for debugging

    surface = new QOffscreenSurface();
    surface->setFormat(format);
    surface->create(); // Safe here - called from main thread
}

void RenderWorker::setup()
{
    running = true;

    Telemetry::breadcrumb("render.setup", "RenderWorker::setup() start");

    // Surface must already be created via initSurface() from main thread
    if (!surface || !surface->isValid()) {
        qFatal("Surface not initialized! Call initSurface() from main thread before run()");
    }

    QSurfaceFormat format = surface->format();

    // create context
    ctx = new QOpenGLContext();
    // https://doc.qt.io/qt-6/qopenglcontext.html#globalShareContext
    ctx->setShareContext(QOpenGLContext::globalShareContext());
    ctx->setFormat(format);
    if (!ctx->create()) {
        reportFatalGlState("render worker context creation failed", {});
        qFatal("unable to create surface!");
    }

    Telemetry::breadcrumb("render.setup", "OpenGL context created, making current");
    ctx->makeCurrent(surface);

    // https://doc-snapshots.qt.io/qt6-dev/gui-changes-qt6.html
    gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_2_Core>(ctx);
    if (!gl) {
        reportFatalGlState("3.2 core functions unavailable on worker context",
                           {{"granted_major",
                             (int64_t)ctx->format().majorVersion()},
                            {"granted_minor",
                             (int64_t)ctx->format().minorVersion()}});
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

    vao->release();

    // create fbo
    // https://doc.qt.io/qt-6/qopenglframebufferobject.html
    // https://www.qt.io/blog/2015/09/21/using-modern-opengl-es-features-with-qopenglframebufferobject-in-qt-5-6
    fbo = new QOpenGLFramebufferObject(TEXTURE_SIZE, TEXTURE_SIZE);
    if (!fbo->isValid()) {
        reportFatalGlState("render worker scratch FBO could not be created",
                           {{"size", (int64_t)TEXTURE_SIZE}});
        qFatal("FBO could not be created");
    }

    // create FBO with no color attachment for depth-only rendering
    fboId = 0;
    gl->glGenFramebuffers(1, &fboId);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    // gl->glDrawBuffer(GL_NONE);
    // gl->glReadBuffer(GL_NONE);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    {
        const QSurfaceFormat granted = ctx->format();
        Telemetry::breadcrumb(
            "render.setup", "RenderWorker::setup() complete",
            {{"granted_version",
              std::to_string(granted.majorVersion()) + "." +
                  std::to_string(granted.minorVersion())},
             {"core_profile",
              granted.profile() == QSurfaceFormat::CoreProfile}});
    }

    // Initialize resource cache for custom node renderers
    resourceCache.init(gl, fboId);

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
    // Deliberately no breadcrumb here: with the Crashpad backend every crumb
    // flushes the scope to disk, and this runs once per node per render pass.
    // The batch-level crumbs in TextureRenderer cover what we actually need.

    if (rdoc_api)
        rdoc_api->StartFrameCapture(NULL, NULL);

    ctx->makeCurrent(surface);

    // Custom renderer path — node defines its own multi-pass rendering.
    // Require renderData too: the renderer immediately downcasts and reads
    // fields off it, so a null (a node that overrides createRenderer() but not
    // createRenderData()) would form a null reference and crash here.
    if (command.renderer && command.renderData) {
        NodeRenderContext renderCtx;
        renderCtx.gl = gl;
        renderCtx.cache = &resourceCache;
        renderCtx.outputTextureId = command.textureId;
        renderCtx.textureWidth = command.textureWidth;
        renderCtx.textureHeight = command.textureHeight;
        renderCtx.randomSeed = command.randomSeed;
        renderCtx.vao = vao;
        renderCtx.vbo = vbo;

        for (const auto& input : command.inputs) {
            renderCtx.inputs.append(
                NodeInputBinding{input.inputName, input.textureId});
        }

        command.renderer->render(renderCtx, *command.renderData);
        resourceCache.releaseAllTextures();

        gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, 0, 0);
        gl->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());

        ctx->doneCurrent();

        if (rdoc_api)
            rdoc_api->EndFrameCapture(NULL, NULL);

        emit nodeRendered(command.nodeId, command.textureId);
        return;
    }

    // CPU processing path
    if (command.usesCpuProcessing && command.nodePtr) {
        TextureNode* node = command.nodePtr.data();
        node->cpuProcess(gl, command);

        ctx->doneCurrent();

        if (rdoc_api)
            rdoc_api->EndFrameCapture(NULL, NULL);

        emit nodeRendered(command.nodeId, command.textureId);
        return;
    }

    // Standard single-pass GPU path
    renderSinglePass(command);

    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, 0, 0);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());

    ctx->doneCurrent();

    if (rdoc_api)
        rdoc_api->EndFrameCapture(NULL, NULL);

    emit nodeRendered(command.nodeId, command.textureId);
}

void RenderWorker::renderSinglePass(const RenderCommand& command)
{
    gl->glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, command.textureId, 0);

    GLenum status = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        reportFatalGlState("render target framebuffer incomplete",
                           {{"status", (int64_t)status},
                            {"width", (int64_t)command.textureWidth},
                            {"height", (int64_t)command.textureHeight}});
        qFatal("FRAMEBUFFER IS NOT COMPLETE!");
    }

    gl->glViewport(0, 0, command.textureWidth, command.textureHeight);

    gl->glClearColor(0, 1, 0, 1);
    gl->glClearDepth(0);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vao->bind();

    if (command.shaderLinked) {
        gl->glUseProgram(command.shaderId);

        // Clear every declared input, not just the connected ones: uniforms
        // live on the shader program, so an input that was connected the last
        // time this node rendered would otherwise keep its stale texture and
        // <name>_connected == true after being disconnected.
        int texIndex = 0;
        for (const auto& inputName : command.inputNames) {
            gl->glActiveTexture(GL_TEXTURE0 + texIndex);
            gl->glBindTexture(GL_TEXTURE_2D, 0);

            gl->glUniform1i(
                gl->glGetUniformLocation(command.shaderId,
                                         inputName.toStdString().c_str()),
                texIndex);
            std::string connectedName = inputName.toStdString() + "_connected";
            gl->glUniform1i(gl->glGetUniformLocation(command.shaderId,
                                                     connectedName.c_str()),
                            0);

            texIndex++;
        }

        // pass inputs
        for (auto nodeInput : command.inputs) {
            auto name = nodeInput.inputName;

            // reuse the unit the clear loop above assigned to this input so
            // the two stay in sync; unknown names get a fresh unit
            int unit = command.inputNames.indexOf(name);
            if (unit < 0)
                unit = texIndex++;

            gl->glActiveTexture(GL_TEXTURE0 + unit);
            gl->glBindTexture(GL_TEXTURE_2D, nodeInput.textureId);

            gl->glUniform1i(gl->glGetUniformLocation(
                                command.shaderId, name.toStdString().c_str()),
                            unit);
            std::string connectedName = name.toStdString() + "_connected";
            gl->glUniform1i(gl->glGetUniformLocation(command.shaderId,
                                                     connectedName.c_str()),
                            1);
        }

        // pass seed
        gl->glUniform1f(gl->glGetUniformLocation(command.shaderId, "_seed"),
                        (GLfloat)(command.randomSeed));

        // texture size
        gl->glUniform2f(
            gl->glGetUniformLocation(command.shaderId, "_textureSize"),
            (GLfloat)(command.textureWidth), (GLfloat)(command.textureHeight));

        // pass props
        for (auto prop : command.props) {
            auto propCString = ("prop_" + prop.propName.toStdString());
            auto propName = propCString.c_str();

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
            case PropType::Gradient: {
                auto gradientVal = prop.value.value<Gradient>();
                auto numPoints = gradientVal.points.size();

                gl->glUniform1i(
                    gl->glGetUniformLocation(
                        command.shaderId, (propCString + ".numPoints").c_str()),
                    numPoints);

                for (int i = 0; i < numPoints; i++) {
                    const auto& point = gradientVal.points[i];
                    const auto& color = point.color;

                    std::string colorPath =
                        propCString + ".colors[" + std::to_string(i) + "]";
                    gl->glUniform3f(gl->glGetUniformLocation(command.shaderId,
                                                             colorPath.c_str()),
                                    color.redF(), color.greenF(),
                                    color.blueF());

                    std::string posPath =
                        propCString + ".positions[" + std::to_string(i) + "]";
                    gl->glUniform1f(gl->glGetUniformLocation(command.shaderId,
                                                             posPath.c_str()),
                                    point.position);
                }
            } break;
            case PropType::Image: {
                if (prop.textureId != 0) {
                    gl->glActiveTexture(GL_TEXTURE0 + texIndex);
                    gl->glBindTexture(GL_TEXTURE_2D, prop.textureId);
                    gl->glUniform1i(
                        gl->glGetUniformLocation(command.shaderId, propName),
                        texIndex);
                    texIndex++;
                }
                else {
                    gl->glActiveTexture(GL_TEXTURE0 + texIndex);
                    gl->glBindTexture(GL_TEXTURE_2D, 0);
                    gl->glUniform1i(
                        gl->glGetUniformLocation(command.shaderId, propName),
                        texIndex);
                    texIndex++;
                }
            } break;
            case PropType::Curve: {
                auto curve   = prop.value.value<Curve>();
                int  nPoints = qMin((int)curve.points.size(), CURVE_MAX_POINTS);

                gl->glUniform1i(
                    gl->glGetUniformLocation(command.shaderId,
                                             (propCString + ".numPoints").c_str()),
                    nPoints);

                for (int i = 0; i < nPoints; i++) {
                    const auto& pt  = curve.points[i];
                    std::string idx = "[" + std::to_string(i) + "]";

                    gl->glUniform2f(
                        gl->glGetUniformLocation(command.shaderId,
                                                 (propCString + ".anchors" + idx).c_str()),
                        pt.x, pt.y);

                    gl->glUniform2f(
                        gl->glGetUniformLocation(command.shaderId,
                                                 (propCString + ".handleR" + idx).c_str()),
                        pt.x + pt.rx, pt.y + pt.ry);

                    gl->glUniform2f(
                        gl->glGetUniformLocation(command.shaderId,
                                                 (propCString + ".handleL" + idx).c_str()),
                        pt.x + pt.lx, pt.y + pt.ly);
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
}

void RenderWorker::kill() { running = false; }
