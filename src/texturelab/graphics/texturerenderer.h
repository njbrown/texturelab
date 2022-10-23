#pragma once

#include <QPixmap>
#include <QSharedPointer>

class QOffscreenSurface;
class QOpenGLContext;
class QOpenGLFunctions_3_2_Core;
class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
class QOpenGLShader;
class QOpenGLFramebufferObject;

class TextureProject;
typedef QSharedPointer<TextureProject> TextureProjectPtr;

// https://stackoverflow.com/questions/31323749/easiest-way-for-offscreen-rendering-with-qopenglwidget
class TextureRenderer {
    QOffscreenSurface* surface;
    QOpenGLContext* ctx;
    QOpenGLFunctions_3_2_Core* gl;
    QOpenGLVertexArrayObject* vao;
    QOpenGLBuffer* vbo;
    QOpenGLShader* vshader;
    QOpenGLShader* fshader;
    QOpenGLFramebufferObject* fbo;

public:
    TextureRenderer();
    void setup();
    void setProject(TextureProjectPtr project);
    void update();
    void testRendering();

    TextureProjectPtr project;
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