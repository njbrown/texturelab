#include "viewer3d.h"
#include <QMatrix4x4>
#include <QOpenGLWindow>
#include <QVector3D>

#include <QImage>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>

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

QOpenGLShaderProgram* createMainShader();
QOpenGLBuffer* loadMesh();

void Viewer3D::initializeGL()
{
    gl = QOpenGLContext::currentContext()->functions();
    gl->initializeOpenGLFunctions();

    vao = new QOpenGLVertexArrayObject;
    if (vao->create())
        vao->bind();

    mainProgram = createMainShader();

    mesh = loadMesh();

    // setup matrices
    worldMatrix.setToIdentity();

    viewMatrix.setToIdentity();
    viewMatrix.lookAt(QVector3D(0, 5, -5), QVector3D(), QVector3D(0, 1, 0));

    projMatrix.setToIdentity();
    projMatrix.perspective(45, this->width() / (float)this->height(), 1.0,
                           1000);
}

void Viewer3D::paintGL()
{
    gl->glViewport(0, 0, this->width(), this->height());
    gl->glClearDepthf(1.0);
    gl->glClearColor(1, 0, 0, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->glEnable(GL_DEPTH_TEST);
    // gl->glEnable(GL_DEPTH_TEST);

    vao->bind();

    // todo: bind textures

    mainProgram->bind();
    mainProgram->setUniformValue("worldMatrix", worldMatrix);
    mainProgram->setUniformValue("viewMatrix", viewMatrix);
    mainProgram->setUniformValue("projMatrix", projMatrix);

    mesh->bind();
    // setup attrib array
    gl->glEnableVertexAttribArray((int)VertexUsage::Position);
    gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
    gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT, GL_FALSE,
                              5 * sizeof(float), nullptr);
    gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT,
                              GL_FALSE, 5 * sizeof(float),
                              reinterpret_cast<void*>(3 * sizeof(float)));

    // render
    gl->glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Viewer3D::resizeGL(int w, int h)
{
    // Calculate aspect ratio
    qreal aspect = qreal(w) / qreal(h ? h : 1);

    // Set near plane to 3.0, far plane to 7.0, field of view 45 degrees
    const qreal zNear = 1.0, zFar = 1000.0, fov = 45.0;

    // Reset projection
    projMatrix.setToIdentity();

    // Set perspective projection
    projMatrix.perspective(fov, aspect, zNear, zFar);
}

QOpenGLShaderProgram* createMainShader()
{
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex);
    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment);
    auto program = new QOpenGLShaderProgram;

    QString vSource = R""""(
        #version 150 core

        in vec3 a_pos;
        in vec3 a_normal;
        in vec3 a_tangent;
        in vec2 a_texCoord;

        uniform mat4 worldMatrix;
        uniform mat4 viewMatrix;
        uniform mat4 projMatrix;

        out vec2 v_texCoord;

        void main()
        {
                v_texCoord = a_texCoord*vec2(1,1);
                vec4 v_pos = projMatrix * viewMatrix * worldMatrix * vec4(a_pos, 1.0);
                gl_Position = v_pos;
        }
    )"""";

    QString fSource = R""""(
        #version 150 core

        in vec2 v_texCoord;
        out vec4 fragColor;

        uniform sampler2D diffuse;

        void main()
        {
            fragColor = texture(diffuse, v_texCoord);
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
    program->bindAttributeLocation("a_normal", (int)VertexUsage::Normal);
    program->bindAttributeLocation("a_tangent", (int)VertexUsage::Tangent);
    program->bindAttributeLocation("a_color", (int)VertexUsage::Color);
    program->bindAttributeLocation("a_texCoord", (int)VertexUsage::TexCoord0);
    program->bindAttributeLocation("a_texCoord1", (int)VertexUsage::TexCoord1);

    if (!program->link()) {
        qDebug() << "SHADER LINK ERROR";
        qDebug() << program->log();

        return nullptr;
    }

    return program;
}

QOpenGLBuffer* loadMesh()
{
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

    auto vbo = new QOpenGLBuffer;
    vbo->create();
    vbo->bind();
    vbo->allocate(data.constData(), data.count() * sizeof(float));
    vbo->release();

    return vbo;
}