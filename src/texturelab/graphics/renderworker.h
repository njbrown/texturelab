#pragma once

#include "../props.h"
#include <QList>
#include <QMutex>
#include <QObject>
#include <QOpenGLFunctions>
#include <QQueue>
#include <atomic>

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
};

struct RenderCommand {
    QString nodeId;
    GLuint shaderId;
    bool shaderLinked;

    GLuint fboId;

    int textureWidth;
    int textureHeight;
    float randomSeed;

    // all expected inputs need to be cleared
    int totalInputs;
    QList<RenderNodeInput> inputs;

    // props
    QList<RenderProp> props;
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

    QMutex mutex;
    std::atomic<bool> running;
    QQueue<RenderCommand> renderQueue;

public:
    RenderWorker();

    void setRenderQueue(QQueue<RenderCommand> renderQueue);
    void run();
    void kill();
    void processRenderCommand(const RenderCommand& command);

private:
    void setup();

signals:
    void nodeRendered(QString nodeId, GLuint textureId);
};