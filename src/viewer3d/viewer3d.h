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

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
};