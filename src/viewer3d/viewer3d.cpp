#include "viewer3d.h"
#include <QMatrix4x4>
#include <QOpenGLWindow>
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

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "gltf/tiny_gltf.h"

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

class Mesh {
public:
    QOpenGLVertexArrayObject* vao = nullptr;
    int numElements = 0;
    std::map<int, QOpenGLBuffer*> vbos;
    QOpenGLBuffer* indexBuffer;
    QList<VertexUsage> attribs;

    tinygltf::Primitive primitive;
    tinygltf::Accessor indexAccessor;

    // transformation props
    QMatrix4x4 transform;
    QMatrix3x3 normalMatrix;
    Material* material = nullptr;
};

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
}

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
    gltfMesh = loadMeshFromRc(":assets/cube.gltf");
    gltfMesh->material = mat;

    // setup matrices
    worldMatrix.setToIdentity();

    viewMatrix.setToIdentity();
    viewMatrix.lookAt(QVector3D(0, 5, -5), QVector3D(), QVector3D(0, 1, 0));

    projMatrix.setToIdentity();
    projMatrix.perspective(45, this->width() / (float)this->height(), 1.0,
                           1000);

    iblSampler = new IblSampler();
    iblSampler->gl = gl;
    iblSampler->init(":assets/panorama.hdr");

    iblSampler->filterAll();

    // do all the conversions
    // iblSampler->panoramaToCubemap();
}

void Viewer3D::paintGL()
{
    buildView();

    gl->glViewport(0, 0, this->width(), this->height());
    gl->glClearDepthf(1.0);
    gl->glClearColor(1, 0, 0, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->glEnable(GL_DEPTH_TEST);
    // gl->glEnable(GL_DEPTH_TEST);

    vao->bind();

    // todo: bind textures
    // iblSampler->inputTexture->bind(0);
    // iblSampler->ggxLutTexture->bind(0);

    // mainProgram->bind();
    // mainProgram->setUniformValue("worldMatrix", worldMatrix);
    // mainProgram->setUniformValue("viewMatrix", viewMatrix);
    // mainProgram->setUniformValue("projMatrix", projMatrix);

    // mesh->bind();
    // // setup attrib array
    // gl->glEnableVertexAttribArray((int)VertexUsage::Position);
    // gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
    // gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT,
    // GL_FALSE,
    //                           5 * sizeof(float), nullptr);
    // gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT,
    //                           GL_FALSE, 5 * sizeof(float),
    //                           reinterpret_cast<void*>(3 * sizeof(float)));

    // // render
    // gl->glDrawArrays(GL_TRIANGLES, 0, 6);

    // render gltf mesh
    renderGltfMesh(gltfMesh);
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

bool loadGltfModel(tinygltf::Model& model, const QString& filename);

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

// https://github.com/syoyo/tinygltf/blob/release/examples/basic/main.cpp
Mesh* loadMeshFromRc(const QString& path)
{
    auto gl = QOpenGLContext::currentContext()->functions();

    tinygltf::Model model;
    if (!loadGltfModel(model, path)) {
        return nullptr;
    }

    // just convert the first mesh
    if (model.meshes.size() == 0)
        return nullptr;

    auto mesh = model.meshes[0];

    auto vao = new QOpenGLVertexArrayObject(nullptr);
    vao->create();
    vao->bind();

    std::map<int, QOpenGLBuffer*> vbos;

    // upload all model buffer views into GPU memory
    for (size_t i = 0; i < model.bufferViews.size(); ++i) {
        const tinygltf::BufferView& bufferView = model.bufferViews[i];
        if (bufferView.target == 0) { // TODO impl drawarrays
            std::cout << "WARN: bufferView.target is zero" << std::endl;
            continue; // Unsupported bufferView.
        }

        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        std::cout << "bufferview.target " << bufferView.target << std::endl;

        // vertex or index buffer
        auto vbo = new QOpenGLBuffer((QOpenGLBuffer::Type)bufferView.target);
        vbo->create();
        vbo->bind();
        vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
        vbo->allocate(&buffer.data.at(0) + bufferView.byteOffset,
                      bufferView.byteLength);
        vbo->release();

        vbos[i] = vbo;

        // GLuint vbo;
        // glGenBuffers(1, &vbo);
        // vbos[i] = vbo;
        // glBindBuffer(bufferView.target, vbo);

        // std::cout << "buffer.data.size = " << buffer.data.size()
        //           << ", bufferview.byteOffset = " << bufferView.byteOffset
        //           << std::endl;

        // glBufferData(bufferView.target, bufferView.byteLength,
        //              &buffer.data.at(0) + bufferView.byteOffset,
        //              GL_STATIC_DRAW);
    }

    tinygltf::Primitive primitive = mesh.primitives[0];
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
    QList<VertexUsage> attribs;

    // assign vertex channels to buffers
    for (auto& attrib : primitive.attributes) {
        tinygltf::Accessor accessor = model.accessors[attrib.second];
        int byteStride =
            accessor.ByteStride(model.bufferViews[accessor.bufferView]);
        // glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);
        vbos[accessor.bufferView]->bind();

        int size = 1;
        if (accessor.type != TINYGLTF_TYPE_SCALAR) {
            size = accessor.type;
        }

        int vaa = -1;
        if (attrib.first.compare("POSITION") == 0)
            vaa = (int)VertexUsage::Position;
        if (attrib.first.compare("NORMAL") == 0)
            vaa = (int)VertexUsage::Normal;
        if (attrib.first.compare("TANGENT") == 0)
            vaa = (int)VertexUsage::Tangent;
        if (attrib.first.compare("TEXCOORD_0") == 0)
            vaa = (int)VertexUsage::TexCoord0;
        if (attrib.first.compare("TEXCOORD_1") == 0)
            vaa = (int)VertexUsage::TexCoord1;
        if (attrib.first.compare("TEXCOORD_2") == 0)
            vaa = (int)VertexUsage::TexCoord2;
        if (vaa > -1) {
            gl->glEnableVertexAttribArray(vaa);
            gl->glVertexAttribPointer(vaa, size, accessor.componentType,
                                      accessor.normalized ? GL_TRUE : GL_FALSE,
                                      byteStride,
                                      BUFFER_OFFSET(accessor.byteOffset));
            attribs.append((VertexUsage)vaa);
        }
        else
            std::cout << "vaa missing: " << attrib.first << std::endl;
    }

    vao->release();

    Mesh* finalMesh = new Mesh;
    finalMesh->vao = vao;
    finalMesh->vbos = vbos;
    finalMesh->attribs = attribs;

    finalMesh->indexBuffer = vbos.at(indexAccessor.bufferView);
    finalMesh->primitive = primitive;
    finalMesh->indexAccessor = indexAccessor;
    return finalMesh;
}

// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/glTF-WebGL-PBR/mesh.js#L113
// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/source/Renderer/renderer.js
void Viewer3D::renderGltfMesh(Mesh* mesh)
{
    // setup material
    auto& mat = mesh->material;
    auto& shader = mat->shader;
    shader->bind();

    QMatrix4x4 mvp;
    mvp.setToIdentity();
    mvp = projMatrix * viewMatrix * worldMatrix;

    auto modelInverse = worldMatrix.inverted();
    auto normalMatrix = modelInverse.transposed();

    shader->setUniformValue("u_ViewProjectionMatrix", projMatrix * viewMatrix);
    shader->setUniformValue("u_ModelMatrix", worldMatrix);
    shader->setUniformValue("u_NormalMatrix", normalMatrix);
    shader->setUniformValue("u_Exposure", 1.0f);
    shader->setUniformValue("u_Camera", camPos);

    // default mat props
    shader->setUniformValue("u_BaseColorFactor", QVector4D(1, 1, 1, 1));
    shader->setUniformValue("u_MetallicFactor", 0.f);
    shader->setUniformValue("u_RoughnessFactor", 1.f);
    shader->setUniformValue("u_EmissiveStrength", 1.f);
    shader->setUniformValue("u_NormalScale", 1.0f);
    shader->setUniformValue("u_BaseColorUVSet", 0);
    shader->setUniformValue("u_NormalUVSet", 0);
    shader->setUniformValue("u_EmissiveUVSet", 0);
    shader->setUniformValue("u_MetallicRoughnessUVSet", 0);

    // albedo
    // mainProgram->setUniformValue("u_BaseColorFactor", mat->albedo);
    shader->setUniformValue("u_BaseColorSampler", 0);
    mat->albedoMap->bind(0);
    // shader->setUniformValue("u_NormalSampler", 1);
    // mat->normalMap->bind(1);
    // shader->setUniformValue("u_MetallicRoughnessSampler", 2);
    // mat->metalnessMap->bind(2);
    // shader->setUniformValue("u_EmissiveSampler", 2);
    // mat->emissiveMap->bind(3);

    // pbr maps - they start at 8
    // https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/source/Renderer/renderer.js#L732
    shader->setUniformValue("u_LambertianEnvSampler", 8);
    // iblSampler->lambertianTexture->bind(8);
    gl->glActiveTexture(GL_TEXTURE0 + 8);
    gl->glBindTexture(GL_TEXTURE_CUBE_MAP, iblSampler->lambertianTextureID);
    shader->setUniformValue("u_GGXEnvSampler", 9);
    // iblSampler->ggxTexture->bind(9);
    gl->glActiveTexture(GL_TEXTURE0 + 9);
    gl->glBindTexture(GL_TEXTURE_CUBE_MAP, iblSampler->ggxTextureID);
    shader->setUniformValue("u_GGXLUT", 10);
    // iblSampler->ggxLutTexture->bind(10);
    gl->glActiveTexture(GL_TEXTURE0 + 10);
    gl->glBindTexture(GL_TEXTURE_2D, iblSampler->ggxLutTextureID);
    shader->setUniformValue("u_CharlieEnvSampler", 11);
    // iblSampler->sheenTexture->bind(11);
    gl->glActiveTexture(GL_TEXTURE0 + 11);
    gl->glBindTexture(GL_TEXTURE_CUBE_MAP, iblSampler->sheenTextureID);
    shader->setUniformValue("u_CharlieLUT", 12);
    // iblSampler->charlieLutTexture->bind(12);
    gl->glActiveTexture(GL_TEXTURE0 + 12);
    gl->glBindTexture(GL_TEXTURE_2D, iblSampler->charlieLutTextureID);

    shader->setUniformValue("u_MipCount", iblSampler->mipmapLevels);
    QMatrix3x3 envRot;
    envRot.setToIdentity();
    shader->setUniformValue("u_EnvRotation", envRot);
    shader->setUniformValue("u_EnvIntensity", 1.0f);

    // render mesh
    mesh->vao->bind();
    tinygltf::Primitive primitive = mesh->primitive;
    tinygltf::Accessor indexAccessor = mesh->indexAccessor;

    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos.at(indexAccessor.bufferView));
    mesh->indexBuffer->bind();

    gl->glEnableVertexAttribArray((int)VertexUsage::Position);
    gl->glEnableVertexAttribArray((int)VertexUsage::Normal);
    gl->glEnableVertexAttribArray((int)VertexUsage::Tangent);
    gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);

    glDrawElements(primitive.mode, indexAccessor.count,
                   indexAccessor.componentType,
                   BUFFER_OFFSET(indexAccessor.byteOffset));

    mesh->vao->release();
}

bool loadGltfModel(tinygltf::Model& model, const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "filenot opened \n";
        return false;
    }

    auto text = file.readAll().toStdString();

    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool res = loader.LoadASCIIFromString(&model, &err, &warn, text.c_str(),
                                          text.length(), "");
    // bool res = loader.LoadASCIIFromFile(&model, &err, &warn, text.c_str(),
    //                                     text.length());
    if (!warn.empty()) {
        std::cout << "WARN: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cout << "ERR: " << err << std::endl;
    }

    if (!res)
        std::cout << "Failed to load glTF: " << filename.toStdString()
                  << std::endl;
    else
        std::cout << "Loaded glTF: " << filename.toStdString() << std::endl;

    return res;
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
    // flags << "HAS_TANGENT_VEC4 1";
    // flags << "DEBUG_NORMAL_GEOMETRY 1";
    flags << "DEBUG_NONE 1"; // IMPORTANT!! caused many headaches..
    flags << "DEBUG DEBUG_NONE";
    // flags << "DEBUG DEBUG_METALLIC_ROUGHNESS";

    flags << "HAS_TEXCOORD_0_VEC2 1";
    flags << "HAS_BASE_COLOR_MAP 1";
    // flags << "HAS_NORMAL_MAP 1";
    // flags << "HAS_METALLIC_ROUGHNESS_MAP 1";
    // flags << "HAS_EMISSIVE_MAP 1";
    flags << "MATERIAL_METALLICROUGHNESS 1"; // MR mode

    flags << "ALPHAMODE_OPAQUE 0";
    flags << "ALPHAMODE_MASK 1";
    flags << "ALPHAMODE_BLEND 2";
    flags << "ALPHAMODE ALPHAMODE_OPAQUE";
    flags << "TONEMAP_ACES_NARKOWICZ 1";
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
    // mat->albedoMap = loadTexture(":assets/brick.jpg");
    mat->albedoMap = loadTexture(":assets/Default_albedo.jpg");
    mat->normalMap = loadTexture(":assets/Default_normal.jpg");
    mat->metalnessMap = loadTexture(":assets/Default_metalRoughness.jpg");
    mat->emissiveMap = loadTexture(":assets/Default_emissive.jpg");

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

// void Viewer3D::renderMesh(Mesh* mesh) {

// }