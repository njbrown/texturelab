#include "texturerenderer.h"
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
// Qt6 only!!
#include <QOpenGLVersionFunctionsFactory>
// #include <QOpenGLPaintDevice>
// #include <QtGui/QOpenGLFunctions_3_3_Core>

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
    fbo = new QOpenGLFramebufferObject(TEXTURE_SIZE, TEXTURE_SIZE);
    if (!fbo->isValid()) {
        qFatal("FBO could not be created");
    }
}

TextureRenderer::TextureRenderer() { this->setup(); }

// void TextureRenderer::setProject(TextureProjectPtr project)
// {
//     this->project = project;
// }

// void TextureRenderer::update()
// {
//     if (!project)
//         return;

//     // check for nodes that need updating and update
//     for (auto& node : project->nodes) {
//         if (!node->isGraphicsInitialized()) {
//             // create texture
//             createNodeTexture(node);
//         }

//         // if the resolution has changed, resize texture
//         if (project->textureWidth != node->texture->width ||
//             project->textureHeight != node->texture->height) {
//             // resize
//             resizeNodeTexture(node);
//         }

//         // if (node->needsUpdate()) {

//         //     // process
//         // }
//     }

//     // todo: use quota
//     while (true) {
//         auto& nextNode = getNextNodeToBeUpdated(project);
//         if (!nextNode)
//             break;

//         renderNode(nextNode);
//     }
// }