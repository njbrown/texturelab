#include "viewer3d.h"
#include "thememanager.h"
#include "tokens.h"
#include <QMatrix4x4>
#include <QOpenGLWindow>
#include <QQuaternion>
#include <QVector3D>

#include <QFile>
#include <QImage>
#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>

#include <iostream>

#include "iblsampler.h"
#include "shadercache.h"

#include "geometry/geometry.h"
#include "renderer/renderer.h"

QOpenGLShaderProgram* createMainShader();
QOpenGLBuffer* loadMesh();
Mesh* loadMeshFromRc(const QString& path);
void renderGltfMesh(Mesh* mesh);

Viewer3D::Viewer3D() : QOpenGLWidget()
{
    // QOpenGLContext* context = new QOpenGLContext;
    // context->setFormat(QSurfaceFormat::defaultFormat());
    // context->create();
    // this->setContext(context);

    Q_INIT_RESOURCE(viewerassets);
}

void Viewer3D::reRender() { this->update(); }

void Viewer3D::initializeGL()
{
    gl = QOpenGLContext::currentContext()->functions();
    gl->initializeOpenGLFunctions();

    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    QOpenGLDebugLogger* logger = new QOpenGLDebugLogger(this);
    logger->initialize();
    QObject::connect(logger, &QOpenGLDebugLogger::messageLogged,
                     [=](const QOpenGLDebugMessage& debugMessage) {
                         qDebug() << debugMessage;
                     });

    logger->startLogging();

    vao = new QOpenGLVertexArrayObject;
    if (vao->create())
        vao->bind();

    shaderCache = new ShaderCache();
    shaderCache->addShaderFile("animation.glsl", ":assets/animation.glsl");
    shaderCache->addShaderFile("brdf.glsl", ":assets/brdf.glsl");
    shaderCache->addShaderFile("functions.glsl", ":assets/functions.glsl");
    shaderCache->addShaderFile("iridiscence.glsl", ":assets/iridiscence.glsl");
    shaderCache->addShaderFile("ibl.glsl", ":assets/ibl.glsl");
    shaderCache->addShaderFile("material_info.glsl",
                               ":assets/material_info.glsl");
    shaderCache->addShaderFile("pbr.frag", ":assets/pbr.frag");
    shaderCache->addShaderFile("primitive.vert", ":assets/primitive.vert");
    shaderCache->addShaderFile("punctual.glsl", ":assets/punctual.glsl");
    shaderCache->addShaderFile("textures.glsl", ":assets/textures.glsl");
    shaderCache->addShaderFile("tonemapping.glsl", ":assets/tonemapping.glsl");

    // mainProgram = createMainShader();

    // mesh = loadMesh();
    auto mat = this->loadMaterial();
    // mat->albedo = QVector4D(0.7, 0.7, 0.7, 1.0);
    mat->roughness = 0.5; // Three.js default
    mat->metalness = 0.0; // Three.js default
    // gltfMesh = loadMeshFromRc(":assets/cube.gltf");
    gltfMesh = createSphere(this->gl, 2, 1000, 1000);
    this->material = mat;

    // Create skydome for rendering environment
    skydomeMesh = createSkydome(this->gl, 100, 32, 16);

    // setup matrices
    worldMatrix.setToIdentity();

    viewMatrix.setToIdentity();
    viewMatrix.lookAt(QVector3D(0, 5, -5), QVector3D(), QVector3D(0, 1, 0));

    projMatrix.setToIdentity();
    projMatrix.perspective(45, this->width() / (float)this->height(), 1.0,
                           1000);

    // iblSampler = new IblSampler();
    // iblSampler->gl = gl;
    // iblSampler->init(":assets/panorama.hdr");
    // iblSampler->filterAll();

    renderer = new Renderer();
    renderer->init(this->gl);
    if (!defaultEnvPath.isEmpty())
        renderer->loadEnvironment(defaultEnvPath);
    else
        renderer->loadEnvironment(":env/studio_kontrast_03_1k.hdr");
}

void Viewer3D::setDefaultEnvironment(const QString path)
{
    this->defaultEnvPath = path;
}

void Viewer3D::paintGL()
{
    buildView();

    // this is already done by QOpenGLWidget
    // also, the supplied width and height are incorrect
    // gl->glViewport(0, 0, this->width(), this->height());
    gl->glClearDepthf(1.0);
    // Themed clear color; read each frame so it follows --dev-theme hot-reload.
    const QColor clear = ThemeManager::instance().theme().color(Tokens::View3dClear);
    gl->glClearColor(clear.redF(), clear.greenF(), clear.blueF(), 1);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->glEnable(GL_DEPTH_TEST);
    gl->glDisable(GL_CULL_FACE);

    vao->bind();

    // Render skydome first (as background)
    if (skydomeMesh) {
        gl->glDepthFunc(GL_LEQUAL);
        renderer->renderSkybox(skydomeMesh, viewMatrix, projMatrix);
        gl->glDepthFunc(GL_LESS);
    }

    // render gltf mesh, double-sided: draw the back faces first and the
    // front faces second so alpha-blended fragments composite back-to-front
    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->glEnable(GL_CULL_FACE);

    if (gltfMesh) {
        gl->glCullFace(GL_FRONT);
        renderer->renderGltfMesh(gltfMesh, material, camPos, worldMatrix,
                                 viewMatrix, projMatrix);

        gl->glCullFace(GL_BACK);
        renderer->renderGltfMesh(gltfMesh, material, camPos, worldMatrix,
                                 viewMatrix, projMatrix);
    }

    gl->glDisable(GL_CULL_FACE);
    gl->glDisable(GL_BLEND);

    // test several in a row
    // int totalSpheres = 6;
    // float roughness = 0;
    // float rinc = 1.0 / (totalSpheres - 1);
    // float x = -12;
    // material->metalness = 1;
    // material->roughness = 0;
    // for (int i = 0; i < totalSpheres; i++) {

    //     worldMatrix.setColumn(3, QVector4D(x, 0, 0, 1));
    //     x += 4;

    //     material->roughness = roughness * roughness;
    //     renderer->renderGltfMesh(gltfMesh, material, camPos, worldMatrix,
    //                              viewMatrix, projMatrix);

    //     roughness += rinc;
    // }

    vao->bind();
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

void Viewer3D::mousePressEvent(QMouseEvent* e)
{
    prevPos = e->pos();

    if (e->button() == Qt::LeftButton) {
        leftMouseDown = true;
    }
    if (e->button() == Qt::MiddleButton) {
        middleMouseDown = true;
    }
}
void Viewer3D::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        leftMouseDown = true;
    }
    if (e->button() == Qt::MiddleButton) {
        middleMouseDown = true;
    }
}
void Viewer3D::mouseMoveEvent(QMouseEvent* e)
{
    if (leftMouseDown) {
        auto diff = e->pos() - prevPos;

        yaw += -diff.x() * dragSpeed;
        pitch += diff.y() * dragSpeed;

        prevPos = e->pos();

        this->repaint();
    }
}

void Viewer3D::wheelEvent(QWheelEvent* e)
{
    auto dir = e->angleDelta().y() > 0 ? -1 : 1;
    zoom += dir * zoomSpeed;

    this->repaint();
}

void Viewer3D::buildView()
{
    QVector3D view(0, 0, -zoom);
    // auto rot = QMatrix4x4::rotate();
    auto rot = QQuaternion::fromEulerAngles(pitch, yaw, 0);
    auto eyePos = rot.rotatedVector(view);

    // offset by center
    eyePos += center;
    camPos = eyePos;

    viewMatrix.setToIdentity();
    viewMatrix.lookAt(eyePos, center, QVector3D(0, 1, 0));
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

Material* Viewer3D::loadMaterial()
{
    auto mat = new Material();

    QStringList flags;
    flags << "USE_IBL 1";
    flags << "HAS_NORMAL_VEC3 1";
    // flags << "LINEAR_OUTPUT 1";
    // flags << "TEST 1";
    // flags << "TEST2 1";
    flags << "HAS_TANGENT_VEC4 1";
    // flags << "DEBUG_NORMAL_GEOMETRY 1";
    flags << "DEBUG_NONE 1"; // IMPORTANT!! caused many headaches..
    flags << "DEBUG DEBUG_NONE";
    // flags << "DEBUG DEBUG_METALLIC_ROUGHNESS";

    flags << "HAS_TEXCOORD_0_VEC2 1";
    flags << "HAS_BASE_COLOR_MAP 1";
    // flags << "HAS_NORMAL_MAP 1";
    // flags << "HAS_ROUGHNESS_MAP 1";
    // flags << "HAS_METALNESS_MAP 1";
    // flags << "HAS_METALLIC_ROUGHNESS_MAP 1";
    // flags << "HAS_EMISSIVE_MAP 1";
    flags << "MATERIAL_METALLICROUGHNESS 1"; // MR mode

    flags << "ALPHAMODE_OPAQUE 0";
    flags << "ALPHAMODE_MASK 1";
    flags << "ALPHAMODE_BLEND 2";
    flags << "ALPHAMODE ALPHAMODE_OPAQUE";
    // flags << "TONEMAP_ACES_NARKOWICZ 1";
    // flags << "TONEMAP_ACES_HILL_EXPOSURE_BOOST 1";

    auto vertShader =
        shaderCache->generateShaderSource("primitive.vert", flags);
    auto fragShader = shaderCache->generateShaderSource("pbr.frag", flags);

    auto shader = new QOpenGLShaderProgram;
    shader->bind();
    shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertShader);
    shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragShader);

    shader->bindAttributeLocation("a_position", (int)VertexUsage::Position);
    shader->bindAttributeLocation("a_normal", (int)VertexUsage::Normal);
    shader->bindAttributeLocation("a_tangent", (int)VertexUsage::Tangent);
    shader->bindAttributeLocation("a_color_0", (int)VertexUsage::Color);
    shader->bindAttributeLocation("a_texcoord_0", (int)VertexUsage::TexCoord0);
    shader->bindAttributeLocation("a_texcoord_1", (int)VertexUsage::TexCoord1);

    shader->link();
    // shader->release();

    mat->shader = shader;

    // textures
    // mat->albedoMapId = loadTextureId(":assets/brick.jpg");
    // mat->albedoMap = loadTexture(":assets/brick.jpg");
    // mat->albedoMap = loadTexture(":assets/albedo.jpg");
    // mat->normalMap = loadTexture(":assets/normal.jpg");
    // mat->metalnessMap = loadTexture(":assets/metalness.jpg");
    // mat->roughnessMap = loadTexture(":assets/roughness.jpg");

    return mat;
}
QOpenGLTexture* Viewer3D::loadTexture(const QString& path)
{
    QImage image(path);
    auto tex = new QOpenGLTexture(image);
    tex->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear,
                          QOpenGLTexture::Linear);
    tex->generateMipMaps();

    return tex;
}

GLuint Viewer3D::loadTextureId(const QString& path)
{
    QImage image(path);
    auto tex = new QOpenGLTexture(image);
    tex->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear,
                          QOpenGLTexture::Linear);
    tex->generateMipMaps();

    return tex->textureId();
}

void Viewer3D::setAlbedoTexture(GLuint texId)
{
    this->material->albedoMapId = texId;
    this->material->needsUpdate = true;
}

void Viewer3D::clearAlbedoTexture()
{
    if (!this->material)
        return;
    this->material->albedoMapId = 0;
    this->material->needsUpdate = true;
}

void Viewer3D::setNormalTexture(GLuint texId)
{
    this->material->normalMapId = texId;
    this->material->needsUpdate = true;
}

void Viewer3D::clearNormalTexture()
{
    if (!this->material)
        return;
    this->material->normalMapId = 0;
    this->material->needsUpdate = true;
}

void Viewer3D::setMetalnessTexture(GLuint texId)
{
    this->material->metalnessMapId = texId;
    this->material->needsUpdate = true;
}

void Viewer3D::clearMetalnessTexture()
{
    if (!this->material)
        return;
    this->material->metalnessMapId = 0;
    this->material->needsUpdate = true;
}

void Viewer3D::setRoughnessTexture(GLuint texId)
{
    this->material->roughnessMapId = texId;
    this->material->needsUpdate = true;
}

void Viewer3D::clearRoughnessTexture()
{
    if (!this->material)
        return;
    this->material->roughnessMapId = 0;
    this->material->needsUpdate = true;
}

void Viewer3D::setHeightTexture(GLuint texId)
{
    this->material->heightMapId = texId;
    this->material->needsUpdate = true;
}

void Viewer3D::clearHeightTexture()
{
    if (!this->material)
        return;
    this->material->heightMapId = 0;
    this->material->needsUpdate = true;
}

void Viewer3D::setHeightScale(float scale)
{
    this->material->heightScale = scale;
    this->material->needsUpdate = true;
}

void Viewer3D::setAoTexture(GLuint texId)
{
    this->material->aoMapId = texId;
    this->material->needsUpdate = true;
}

void Viewer3D::clearAoTexture()
{
    if (!this->material)
        return;
    this->material->aoMapId = 0;
    this->material->needsUpdate = true;
}

void Viewer3D::setAlphaTexture(GLuint texId)
{
    this->material->alphaMapId = texId;
    this->material->needsUpdate = true;
}

void Viewer3D::clearAlphaTexture()
{
    if (!this->material)
        return;
    this->material->alphaMapId = 0;
    this->material->needsUpdate = true;
}

void Viewer3D::clearTextures()
{
    this->clearAlbedoTexture();
    this->clearNormalTexture();
    this->clearMetalnessTexture();
    this->clearRoughnessTexture();
    this->clearHeightTexture();
    this->clearAoTexture();
    this->clearAlphaTexture();
}

void Viewer3D::resetCamera()
{
    zoom = 7;
    yaw = 0;
    pitch = 0;
    center = QVector3D(0, 0, 0);
    this->repaint();
}

void Viewer3D::loadEnvironment(const QString path)
{
    if (renderer) {
        renderer->loadEnvironment(path);
        this->repaint();
    }
}

void Viewer3D::setModel(const QString& modelType)
{
    // Bind OpenGL context
    makeCurrent();

    // Build the new mesh into a local first; only swap in (and free the old
    // one) once creation succeeds, so a failed allocation can't leave gltfMesh
    // null or delete the current mesh prematurely.
    Mesh* newMesh = nullptr;
    if (modelType == "sphere") {
        newMesh = createSphere(this->gl, 2, 1000, 1000);
    }
    else if (modelType == "plane_xy") {
        // Create a subdivided plane in XY orientation
        newMesh = createPlane(this->gl, 4, 4, 1000, 1000, PlaneOrientation::XY);
    }
    else if (modelType == "plane_yz") {
        // Create a subdivided plane in YZ orientation
        newMesh = createPlane(this->gl, 4, 4, 1000, 1000, PlaneOrientation::YZ);
    }
    else if (modelType == "plane_xz") {
        // Create a subdivided plane in XZ orientation
        newMesh = createPlane(this->gl, 4, 4, 1000, 1000, PlaneOrientation::XZ);
    }
    else if (modelType == "cylinder") {
        // Create a cylinder with height subdivisions for displacement mapping
        newMesh = createCylinder(this->gl, 1, 1, 2, 1000, 1000, 0.1f, 16);
    }
    else if (modelType == "cube") {
        // Create a subdivided cube
        newMesh = createCube(this->gl, 2, 2, 2, 1000, 1000, 1000);
    }
    else if (modelType == "cubesphere") {
        // CubeSphere - a sphere with low segments for a more cubic look
        newMesh = createSphere(this->gl, 2, 8, 8);
    }
    else {
        // Default to sphere
        newMesh = createSphere(this->gl, 2, 1000, 1000);
    }

    if (newMesh) {
        delete gltfMesh;
        gltfMesh = newMesh;
    }

    // Release OpenGL context
    doneCurrent();

    this->repaint();
}