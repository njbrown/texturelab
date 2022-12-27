#include "../renderer/renderer.h"
#include "geometry.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <algorithm>
#include <math.h>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

Mesh* createSphere(QOpenGLFunctions* gl, float radius, int widthSegments,
                   int heightSegments, float phiStart, float phiLength,
                   float thetaStart, float thetaLength)
{
    const float uvScaleX = 2;
    const float uvScaleY = 1;

    widthSegments = std::max(3.0, std::floor(widthSegments));
    heightSegments = std::max(2.0, std::floor(heightSegments));

    auto thetaEnd = std::min((double)thetaStart + thetaLength, M_PI);

    int index = 0;
    QVector<QVector<unsigned int>> grid;

    QVector3D vertex;

    // buffers
    QVector<unsigned int> indices;
    QVector<float> vertices;
    QVector<float> normals;
    QVector<float> uvs;

    for (int iy = 0; iy <= heightSegments; iy++) {
        QVector<unsigned int> verticesRow;

        auto v = iy / (float)heightSegments;

        auto uOffset = 0;
        if (iy == 0 && thetaStart == 0) {

            uOffset = 0.5 / widthSegments;
        }
        else if (iy == heightSegments && thetaEnd == M_PI) {

            uOffset = -0.5 / widthSegments;
        }

        for (int ix = 0; ix <= widthSegments; ix++) {

            auto u = ix / (float)widthSegments;

            // vertex

            auto x = -radius * std::cos(phiStart + u * phiLength) *
                     std::sin(thetaStart + v * thetaLength);
            auto y = radius * std::cos(thetaStart + v * thetaLength);
            auto z = radius * std::sin(phiStart + u * phiLength) *
                     std::sin(thetaStart + v * thetaLength);

            vertices.append({x, y, z});

            // normal
            QVector3D normal(x, y, z);
            normal.normalize();
            normals.append({normal.x(), normal.y(), normal.z()});

            // uv

            uvs.append({(u + uOffset) * uvScaleX, (1 - v) * uvScaleY});

            verticesRow.append(index++);
        }

        grid.append(verticesRow);
    }

    for (int iy = 0; iy < heightSegments; iy++) {

        for (int ix = 0; ix < widthSegments; ix++) {

            auto a = grid[iy][ix + 1];
            auto b = grid[iy][ix];
            auto c = grid[iy + 1][ix];
            auto d = grid[iy + 1][ix + 1];

            if (iy != 0 || thetaStart > 0)
                indices.append({a, b, d});
            if (iy != heightSegments - 1 || thetaEnd < M_PI)
                indices.append({b, c, d});
        }
    }

    // build
    QOpenGLVertexArrayObject* vao = new QOpenGLVertexArrayObject();
    vao->create();
    vao->bind();

    QOpenGLBuffer* vbo;

    // position
    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo->allocate(vertices.data(), vertices.length() * sizeof(float));
    gl->glEnableVertexAttribArray((int)VertexUsage::Position);
    gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT, GL_FALSE,
                              3 * sizeof(float), BUFFER_OFFSET(0));

    // normal
    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo->allocate(normals.data(), normals.length() * sizeof(float));
    gl->glEnableVertexAttribArray((int)VertexUsage::Normal);
    gl->glVertexAttribPointer((int)VertexUsage::Normal, 3, GL_FLOAT, GL_FALSE,
                              3 * sizeof(float), BUFFER_OFFSET(0));

    // uv
    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo->allocate(uvs.data(), uvs.length() * sizeof(float));
    gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
    gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT,
                              GL_FALSE, 2 * sizeof(float), BUFFER_OFFSET(0));

    vao->release();

    // indices
    auto ibo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ibo->create();
    ibo->bind();
    ibo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    ibo->allocate(indices.data(), indices.length() * sizeof(unsigned int));

    auto mesh = new Mesh();
    mesh->vao = vao;
    mesh->meshType = MeshType::Generated;
    mesh->indexBuffer = ibo;
    mesh->numElements = indices.count();
    mesh->indexByteOffset = 0;
    mesh->indexType = GL_UNSIGNED_INT;
    mesh->primitiveMode = GL_TRIANGLES;

    return mesh;
}