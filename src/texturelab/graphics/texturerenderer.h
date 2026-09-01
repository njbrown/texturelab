#pragma once

#include <QOpenGLContext>
#include <QPixmap>
#include <QSharedPointer>
#include <QThread>

#include "../systeminfo.h"

#include <cstdint>

class QOffscreenSurface;
class QOpenGLContext;
class QOpenGLFunctions_3_2_Core;
class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
class QOpenGLShader;
class QOpenGLShaderProgram;
class QOpenGLFramebufferObject;

class RenderWorker;
class TextureProject;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
class TextureNode;
typedef QSharedPointer<TextureNode> TextureNodePtr;

struct NodeInput {
    TextureNodePtr node;
    QString name;
};

// https://stackoverflow.com/questions/31323749/easiest-way-for-offscreen-rendering-with-qopenglwidget
class TextureRenderer : public QObject {
    Q_OBJECT

    QOpenGLFunctions_3_2_Core* gl;
    QOpenGLVertexArrayObject* vao;
    QOpenGLBuffer* vbo;
    QOpenGLShader* vshader;
    QOpenGLShader* fshader;
    QOpenGLFramebufferObject* fbo;

    QThread* renderThread;
    RenderWorker* renderWorker;

    // True from the moment a RenderCommand is handed to the worker until its
    // nodeRendered() callback fires. GUI-thread only. While true, node
    // textures/FBOs must not be resized/recreated: their GLuints may be
    // captured as inputs in the in-flight command.
    bool renderInFlight = false;

    // The last resolution every node was successfully allocated at. A failed
    // resize (4K on a small-VRAM GPU is the case that prompted this) rolls the
    // project back to it instead of aborting the process.
    int lastGoodResolution = 0;

public:
    TextureRenderer();
    ~TextureRenderer();
    void setup();
    void setProject(TextureProjectPtr project);
    void update();
    void updateOld();
    void testRendering();

    // False when the node's texture could not be allocated.
    bool initializeNodeGraphicsResources(const TextureNodePtr& node);
    // False when the FBO could not be allocated — almost always the GPU being
    // out of memory for a w*h*16-byte RGBA32F target per node.
    bool createNodeTexture(const TextureNodePtr& node);
    void renderNode(const TextureNodePtr& node);

    // Free/total VRAM as the driver reports it, queried on this renderer's own
    // context. Callers on the GUI thread generally have no context current, and
    // SystemInfo::queryGpuMemory() needs one — going through here rather than
    // relying on whichever context a widget happened to leave bound.
    SystemInfo::GpuMemory queryGpuMemory();

    // Bytes of VRAM one node's texture needs at the given square resolution.
    // RGBA32F: 4 channels x 4 bytes.
    static int64_t estimatedNodeTextureBytes(int resolution)
    {
        return (int64_t)resolution * resolution * 16;
    }

    TextureProjectPtr project;
    QOffscreenSurface* surface;
    QOpenGLContext* ctx;

private:
    // Retreat to lastGoodResolution after a failed allocation batch, re-allocate
    // there, and emit resolutionChangeFailed().
    void rollBackResolution(int requested);

    void initRenderWorker();
    void nodeRendered(const QString& nodeId, GLuint texId);
    void queueNextNodeToRender();

    QVector<NodeInput> getNodeInputs(const TextureNodePtr& node);
    TextureNodePtr getNextUpdatableNode() const;
    QOpenGLShaderProgram* buildShaderForNode(const TextureNodePtr& node);

signals:
    void thumbnailGenerated(const QString& nodeId, GLuint texId,
                            const QPixmap& pixmap);
    void renderProgress(int clean, int total);

    // The project could not be allocated at `requested`; it has been rolled
    // back to `fallback` and is rendering normally again. GraphWidget uses this
    // to tell the user and put the resolution picker back.
    void resolutionChangeFailed(int requested, int fallback);
};

// note: there's no specified fbo limit
// https://stackoverflow.com/a/41164761
class NodeTexture {
public:
    // texture id
    int width;
    int height;

    QPixmap thumbnail;
};