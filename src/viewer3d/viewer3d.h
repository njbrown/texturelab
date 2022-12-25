#pragma once

#include <QMatrix4x4>
#include <QOpenGLWidget>
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

class QOpenGLTexture;
class QOpenGLShaderProgram;
class QOpenGLBuffer;
class QOpenGLVertexArrayObject;
class Mesh;
class ShaderCache;

class IblSampler;

struct Material {
    QOpenGLShaderProgram* shader = nullptr;

    QOpenGLTexture* albedoMap = nullptr;
    QOpenGLTexture* normalMap = nullptr;
    QOpenGLTexture* metalnessMap = nullptr;
    QOpenGLTexture* roughnessMap = nullptr;
    QOpenGLTexture* heightMap = nullptr;
    QOpenGLTexture* aoMap = nullptr;
    QOpenGLTexture* emissiveMap = nullptr;
    QOpenGLTexture* alphaMap = nullptr;

    QVector3D albedo = QVector3D(1, 1, 1);
    float normalIntensity = 1.0;
    float roughness = 1.0;
    float heightScale = 1.0;
    float alpha = 1.0;
    QVector3D emission = QVector3D(1, 1, 1);
};

class Viewer3D : public QOpenGLWidget {
    QOpenGLTexture* texture = nullptr;
    QOpenGLShaderProgram* mainProgram = nullptr;
    QOpenGLBuffer* mesh = nullptr;
    QOpenGLVertexArrayObject* vao = nullptr;

    Mesh* gltfMesh;

    QOpenGLFunctions* gl = nullptr;

    int m_projMatrixLoc = 0;
    int m_camMatrixLoc = 0;
    int m_worldMatrixLoc = 0;
    int m_myMatrixLoc = 0;
    int m_lightPosLoc = 0;

    QMatrix4x4 projMatrix;
    QMatrix4x4 viewMatrix;
    QMatrix4x4 worldMatrix;

    // QVector3D m_eye;
    // QVector3D m_target = {0, 0, -1};

    // trackball params
    float zoom = 5;
    float zoomSpeed = 0.3f;
    QPoint prevPos;
    QVector3D center = {0, 0, 0};
    QVector3D camPos = {0, 0, 0};
    float yaw = 0;
    float pitch = 0;
    float dragSpeed = 0.5f;

    bool leftMouseDown = false;
    bool middleMouseDown = false;

    IblSampler* iblSampler;
    ShaderCache* shaderCache;

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

    // trackball events
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

    void buildView();

    Material* loadMaterial();
    QOpenGLTexture* loadTexture(const QString& path);
    // void renderMesh(Mesh* mesh);
    void renderGltfMesh(Mesh* mesh);

public:
    Viewer3D();

    // public functions exposed to main app
    void setAlbedoTexture(GLuint texId);
    void setNormalTexture(GLuint texId);
    void setMetalnessTexture(GLuint texId);
    void setRoughnessTexture(GLuint texId);
    void setAlphaTexture(GLuint texId);
    void setAoTexture(GLuint texId);
    void setEmissiveTexture(GLuint texId);
    void setHeightTexture(GLuint texId);

    void resetTextures();
    void resetCamera();
    void loadEnvironment(const QString path);
};