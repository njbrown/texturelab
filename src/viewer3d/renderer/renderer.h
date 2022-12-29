#pragma once

#include <QList>
#include <QMatrix3x3>
#include <QMatrix4x4>
#include <QStringList>
#include <QVector3D>
#include <map>

class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
class Material;
class QOpenGLFunctions;
class QOpenGLTexture;
class QOpenGLShaderProgram;
class IblSampler;
class ShaderCache;

typedef unsigned int GLuint;

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

enum class MeshType { Generated, Gltf };

class MeshPrivate;
class Mesh {
public:
    QOpenGLVertexArrayObject* vao = nullptr;
    std::map<int, QOpenGLBuffer*> vbos;
    QList<VertexUsage> attribs;

    QOpenGLBuffer* indexBuffer = nullptr;
    // int elementCount;
    int numElements = 0;
    int indexByteOffset;
    int primitiveMode; // GL_TRIANGLES
    int indexType;     // most likely GL_INT or GL_UNSIGNED_INT

    // transformation props
    QMatrix4x4 transform;
    QMatrix3x3 normalMatrix;
    Material* material = nullptr;

    // determines how to render (for now)
    // in the future, all meshes should be rendered the same
    MeshType meshType;
};

struct Material {
    QOpenGLShaderProgram* shader = nullptr;

    GLuint albedoMapId = 0;
    GLuint normalMapId = 0;
    GLuint metalnessMapId = 0;
    GLuint roughnessMapId = 0;

    // QOpenGLTexture* albedoMap = nullptr;
    // QOpenGLTexture* normalMap = nullptr;
    // QOpenGLTexture* metalnessMap = nullptr;
    // QOpenGLTexture* roughnessMap = nullptr;
    // QOpenGLTexture* heightMap = nullptr;
    // QOpenGLTexture* aoMap = nullptr;
    // QOpenGLTexture* emissiveMap = nullptr;
    // QOpenGLTexture* alphaMap = nullptr;

    QVector4D albedo = QVector4D(1, 1, 1, 1);
    float metalness = 0.0;
    float normalIntensity = 1.0;
    float roughness = 0.9;
    float heightScale = 1.0;
    float alpha = 1.0;
    QVector3D emission = QVector3D(1, 1, 1);

    QStringList flags;
    bool needsUpdate = true;
};

class Renderer {
public:
    IblSampler* iblSampler;
    ShaderCache* shaderCache;
    QOpenGLFunctions* gl;

    void init(QOpenGLFunctions* gl);
    void loadEnvironment(const QString& path);

    void renderMesh(Mesh* mesh, Material* material);
    void updateMaterial(Material* material);

    void renderGltfMesh(Mesh* mesh, Material* material, const QVector3D& camPos,
                        const QMatrix4x4& worldMatrix,
                        const QMatrix4x4& viewMatrix,
                        const QMatrix4x4& projMatrix);
};

Mesh* loadMeshFromRc(const QString& path);
