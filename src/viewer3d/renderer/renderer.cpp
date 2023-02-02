#include "renderer.h"
#include "../gltf/gltf.cpp"
#include "../iblsampler.h"
#include "../shadercache.h"

#include <QFile>
#include <iostream>

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "../gltf/tiny_gltf.h"

class MeshPrivate {
public:
    tinygltf::Primitive primitive;
    tinygltf::Accessor indexAccessor;
};

void Renderer::init(QOpenGLFunctions* gl)
{
    this->gl = gl;

    // init shader cache
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

    // init ibls
    iblSampler = new IblSampler();
    iblSampler->gl = gl;
}

void Renderer::loadEnvironment(const QString& path)
{
    iblSampler->init(path);
    iblSampler->filterAll();
}

void Renderer::renderMesh(Mesh* mesh, Material* material) {}

void Renderer::updateMaterial(Material* material)
{
    if (material->shader) {
        material->shader->deleteLater();
    }

    // flags
    QStringList flags;

    // the default
    // todo: make this dynamic instead of using flags
    flags << "USE_IBL 1";

    // determine attribs from mesh
    flags << "HAS_NORMAL_VEC3 1";
    flags << "HAS_TEXCOORD_0_VEC2 1";
    // flags << "HAS_TANGENT_VEC4 1";

    flags << "DEBUG_NONE 1"; // IMPORTANT!! caused many headaches..
    flags << "DEBUG DEBUG_NONE";

    if (material->albedoMapId != 0)
        flags << "HAS_BASE_COLOR_MAP 1";
    if (material->normalMapId != 0)
        flags << "HAS_NORMAL_MAP 1";
    if (material->metalnessMapId != 0)
        flags << "HAS_METALNESS_MAP 1";
    if (material->roughnessMapId != 0)
        flags << "HAS_ROUGHNESS_MAP 1";
    // flags << "HAS_NORMAL_MAP 1";
    // flags << "HAS_ROUGHNESS_MAP 1";
    // flags << "HAS_METALNESS_MAP 1";
    // flags << "HAS_METALLIC_ROUGHNESS_MAP 1";
    // flags << "HAS_EMISSIVE_MAP 1";
    flags << "MATERIAL_METALLICROUGHNESS 1"; // MR mode

    // blending
    flags << "ALPHAMODE_OPAQUE 0";
    flags << "ALPHAMODE_MASK 1";
    flags << "ALPHAMODE_BLEND 2";
    flags << "ALPHAMODE ALPHAMODE_OPAQUE";

    // tone mapping
    // flags << "TONEMAP_ACES_NARKOWICZ 1";

    // build shader
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

    material->shader = shader;
    material->needsUpdate = false;
}

#define BUFFER_OFFSET(i) ((char*)NULL + (i))
bool loadGltfModel(tinygltf::Model& model, const QString& filename);

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
    // finalMesh->primitive = primitive;
    // finalMesh->indexAccessor = indexAccessor;
    finalMesh->primitiveMode = primitive.mode;
    finalMesh->numElements = indexAccessor.count;
    finalMesh->indexType = indexAccessor.componentType;
    finalMesh->indexByteOffset = indexAccessor.byteOffset;

    return finalMesh;
}

// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/glTF-WebGL-PBR/mesh.js#L113
// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/source/Renderer/renderer.js
void Renderer::renderGltfMesh(Mesh* mesh, Material* material,
                              const QVector3D& camPos,
                              const QMatrix4x4& worldMatrix,
                              const QMatrix4x4& viewMatrix,
                              const QMatrix4x4& projMatrix)
{
    // setup material
    auto mat = material;
    if (mat->needsUpdate) {
        this->updateMaterial(mat);
    }

    auto shader = mat->shader;
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
    shader->setUniformValue("u_BaseColorFactor", material->albedo);
    shader->setUniformValue("u_MetallicFactor", material->metalness);
    shader->setUniformValue("u_RoughnessFactor", material->roughness);
    shader->setUniformValue("u_EmissiveStrength", 0.f);
    shader->setUniformValue("u_NormalScale", material->normalIntensity);
    // uv sets
    shader->setUniformValue("u_BaseColorUVSet", 0);
    shader->setUniformValue("u_NormalUVSet", 0);
    shader->setUniformValue("u_MetalnessUVSet", 0);
    shader->setUniformValue("u_RoughnessUVSet", 0);
    shader->setUniformValue("u_EmissiveUVSet", 0);
    shader->setUniformValue("u_MetallicRoughnessUVSet", 0);

    if (mat->albedoMapId != 0) {
        shader->setUniformValue("u_BaseColorSampler", 0);
        gl->glActiveTexture(GL_TEXTURE0);
        gl->glBindTexture(GL_TEXTURE_2D, mat->albedoMapId);
    }

    // albedo
    // mainProgram->setUniformValue("u_BaseColorFactor", mat->albedo);
    // shader->setUniformValue("u_BaseColorSampler", 0);
    // mat->albedoMap->bind(0);
    // shader->setUniformValue("u_NormalSampler", 1);
    // mat->normalMap->bind(1);
    // shader->setUniformValue("u_MetalnessSampler", 2);
    // mat->metalnessMap->bind(2);
    // shader->setUniformValue("u_RoughnessSampler", 3);
    // mat->roughnessMap->bind(3);

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
    // tinygltf::Primitive primitive = mesh->primitive;
    // tinygltf::Accessor indexAccessor = mesh->indexAccessor;

    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos.at(indexAccessor.bufferView));
    mesh->indexBuffer->bind();

    // gl->glEnableVertexAttribArray((int)VertexUsage::Position);
    // gl->glEnableVertexAttribArray((int)VertexUsage::Normal);
    // gl->glEnableVertexAttribArray((int)VertexUsage::Tangent);
    // gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);

    // glDrawElements(primitive.mode, indexAccessor.count,
    //                indexAccessor.componentType,
    //                BUFFER_OFFSET(indexAccessor.byteOffset));

    glDrawElements(mesh->primitiveMode, mesh->numElements, mesh->indexType,
                   BUFFER_OFFSET(mesh->indexByteOffset));

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