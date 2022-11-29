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

    QVector3D m_eye;
    QVector3D m_target = {0, 0, -1};

    // trackball params
    float zoom = 5;
    float zoomSpeed = 0.3f;
    QPoint prevPos;
    QVector3D center = {0, 0, 0};
    float yaw = 0;
    float pitch = 0;
    float dragSpeed = 0.5f;

    bool leftMouseDown = false;
    bool middleMouseDown = false;

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
};