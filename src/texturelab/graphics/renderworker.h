#pragma once

#include "../props.h"
#include "noderenderer.h"
#include <QList>
#include <QMutex>
#include <QObject>
#include <QOpenGLFunctions>
#include <QQueue>
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
    void* nodePtr = nullptr; // TextureNode* pointer for CPU processing

    // all expected inputs need to be cleared
    int totalInputs;
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