#pragma once

#include <QList>
#include <QMap>
#include <QPair>
#include <QOpenGLFunctions>
#include <QString>
#include <QStringList>
#include <memory>

class QOpenGLFunctions_3_2_Core;
class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
class QOpenGLShaderProgram;

// Input texture binding passed to custom renderers
struct NodeInputBinding {
    QString name;
    GLuint textureId;
};

// Base class for node-specific render data.
// Nodes subclass this to carry parameters to the render thread.
// Must not reference GPU resources — only plain values.
struct NodeRenderData {
    virtual ~NodeRenderData() = default;
};

// Worker-owned GPU resource pool.
// Manages intermediate textures and compiled shaders.
// All methods must be called on the render thread with GL context active.
class RenderResourceCache {
public:
    RenderResourceCache() = default;

    void init(QOpenGLFunctions_3_2_Core* glFuncs, GLuint fboId);
    void cleanup();

    // --- Intermediate textures ---
    GLuint acquireTexture(int width, int height);
    void releaseTexture(GLuint textureId);
    void releaseAllTextures();

    // --- Shader cache (raw GLSL) ---
    GLuint getOrCompileShader(const QString& key,
                              const QString& vertexSource,
                              const QString& fragmentSource);

    // --- Shader cache (node-style with standard wrapping) ---
    // Wraps processSource with random lib, gradient lib, input/prop declarations.
    // propTypes: list of (name, PropType::Value as int) pairs.
    GLuint compileNodeShader(const QString& key,
                             const QString& processSource,
                             const QStringList& inputNames,
                             const QList<QPair<QString, int>>& propTypes);

    // --- FBO ---
    GLuint fboId() const { return m_fboId; }
    void bindFboToTexture(GLuint textureId);

    // Standard vertex shader source (shared across all node shaders)
    static QString standardVertexSource();

private:
    QOpenGLFunctions_3_2_Core* gl = nullptr;
    GLuint m_fboId = 0;

    struct CachedTexture {
        GLuint id = 0;
        int width = 0;
        int height = 0;
        bool inUse = false;
    };
    QList<CachedTexture> texturePool;

    QMap<QString, QOpenGLShaderProgram*> shaderCache;

    static QString fragmentPreamble();
    static QString randomLib();
    static QString gradientLib();
    static QString generateInputDeclarations(const QStringList& inputNames);
    static QString generatePropDeclarations(
        const QList<QPair<QString, int>>& propTypes);
};

// View into the worker's GL state, passed to renderers at render time.
// Provides helpers for common rendering operations.
struct NodeRenderContext {
    QOpenGLFunctions_3_2_Core* gl;
    RenderResourceCache* cache;

    GLuint outputTextureId;
    int textureWidth;
    int textureHeight;
    float randomSeed;

    QList<NodeInputBinding> inputs;

    QOpenGLVertexArrayObject* vao;
    QOpenGLBuffer* vbo;

    // Bind shader, set viewport, set _textureSize and _seed uniforms
    void useShader(GLuint programId);

    // Bind a texture to a sampler uniform
    void bindTexture(GLuint programId, const QString& uniformName,
                     GLuint textureId, int unit);

    // Draw a fullscreen quad using the currently bound shader
    void drawQuad();
};

// Abstract base for custom node renderers.
// Subclass this to implement multi-pass or custom rendering logic.
// The worker delegates to render() instead of the standard single-pass path.
class NodeTextureRenderer {
public:
    virtual ~NodeTextureRenderer() = default;

    // Called on the render thread with full GL context.
    // Must write final result to context.outputTextureId.
    virtual void render(NodeRenderContext& context,
                        const NodeRenderData& data) = 0;
};
