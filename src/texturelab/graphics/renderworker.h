#pragma once

#include "../props.h"
#include "noderenderer.h"
#include <QList>
#include <QMutex>
#include <QObject>
#include <QOpenGLFunctions>
#include <QQueue>
#include <QSharedPointer>
#include <atomic>
#include <memory>

class QOffscreenSurface;
class QOpenGLContext;
class QOpenGLFunctions_3_2_Core;
class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
class QOpenGLShader;
class QOpenGLShaderProgram;
class QOpenGLFramebufferObject;

class TextureNode;
typedef QSharedPointer<TextureNode> TextureNodePtr;

struct RenderNodeInput {
    QString nodeId;
    QString inputName;
    GLuint textureId;
};

struct RenderProp {
    QString propName;
    PropType::Value propType;
    QVariant value;
    GLuint textureId; // For ImageProp: pre-uploaded texture ID
};

struct RenderCommand {
    QString nodeId;

    GLuint textureId;
    GLuint shaderId;
    bool shaderLinked;

    GLuint fboId;

    int textureWidth;
    int textureHeight;
    float randomSeed;

    // CPU processing support
    bool usesCpuProcessing = false;
    // Shared (not raw) so the node stays alive for the lifetime of this
    // command even if it's removed from the project while queued/in-flight.
    TextureNodePtr nodePtr;

    // Every input the node declares, connected or not. Uniform state lives on
    // the shader program, so an input left over from a previous render still
    // has its sampler and <name>_connected flag set: all declared inputs have
    // to be cleared each render, not just the connected ones.
    QStringList inputNames;

    // only the inputs that currently have a connection
    QList<RenderNodeInput> inputs;

    // props
    QList<RenderProp> props;

    // Custom rendering support (null = standard single-pass)
    std::shared_ptr<NodeTextureRenderer> renderer;
    std::shared_ptr<NodeRenderData> renderData;
};

class RenderWorker : public QObject {
    Q_OBJECT

    QOffscreenSurface* surface;
    QOpenGLContext* ctx;
    QOpenGLFunctions_3_2_Core* gl;
    QOpenGLVertexArrayObject* vao;
    QOpenGLBuffer* vbo;
    QOpenGLShader* vshader;
    QOpenGLShader* fshader;
    QOpenGLFramebufferObject* fbo;

    // use custom dbo that gets shared across render textures
    GLuint fboId;

    RenderResourceCache resourceCache;

    QMutex mutex;
    std::atomic<bool> running;
    QQueue<RenderCommand> renderQueue;

    void renderSinglePass(const RenderCommand& command);

public:
    RenderWorker();

    void setRenderQueue(QQueue<RenderCommand> renderQueue);
    void run();
    void kill();
    void processRenderCommand(const RenderCommand& command);
    void renderNextInQueue();

    // Must be called from main thread before run() - Windows requires surface creation on GUI thread
    void initSurface();
    void setup();

private:
signals:
    void nodeRendered(QString nodeId, GLuint textureId);
};