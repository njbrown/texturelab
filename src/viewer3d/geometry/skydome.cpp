#include "../renderer/renderer.h"
#include "geometry.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <algorithm>
#include <math.h>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

Mesh* createSkydome(QOpenGLFunctions* gl, float radius, int widthSegments,
                    int heightSegments)
{
    widthSegments = std::max(3, (int)std::floor(widthSegments));
    heightSegments = std::max(2, (int)std::floor(heightSegments));

    int index = 0;
    QVector<QVector<unsigned int>> grid;

    // buffers
    QVector<unsigned int> indices;
    QVector<float> vertices;
    QVector<float> normals;
    QVector<float> tangents;
    QVector<float> uvs;

    // Generate vertices for an inverted sphere (skydome)
    for (int iy = 0; iy <= heightSegments; iy++) {
        QVector<unsigned int> verticesRow;

        float v = iy / (float)heightSegments;
        float theta = v * M_PI;

        for (int ix = 0; ix <= widthSegments; ix++) {
            float u = ix / (float)widthSegments;
            float phi = u * M_PI * 2.0f;

            // Vertex position (standard sphere)
            float x = -radius * std::cos(phi) * std::sin(theta);
            float y = radius * std::cos(theta);
            float z = radius * std::sin(phi) * std::sin(theta);

            vertices.append(x);
            vertices.append(y);
            vertices.append(z);

            // Normal pointing inward (inverted)
            QVector3D normal(-x, -y, -z);
            normal.normalize();
            normals.append(normal.x());
            normals.append(normal.y());
            normals.append(normal.z());

            // Tangent
            QVector3D tangent(std::sin(phi), 0.0f, std::cos(phi));
            tangent.normalize();
            tangents.append(tangent.x());
            tangents.append(tangent.y());
            tangents.append(tangent.z());
            tangents.append(1.0f);

            // UV coordinates
            uvs.append(u);
            uvs.append(v);

            verticesRow.append(index++);
        }

        grid.append(verticesRow);
    }

    // Generate indices (reversed winding order for inverted sphere)
    for (int iy = 0; iy < heightSegments; iy++) {
        for (int ix = 0; ix < widthSegments; ix++) {
            unsigned int a = grid[iy][ix + 1];
            unsigned int b = grid[iy][ix];
            unsigned int c = grid[iy + 1][ix];
            unsigned int d = grid[iy + 1][ix + 1];

            // Reversed winding order compared to normal sphere
            if (iy != 0)
                indices.append({b, a, d});
            if (iy != heightSegments - 1)
                indices.append({c, b, d});
        }
    }

    // Build OpenGL buffers
    QOpenGLVertexArrayObject* vao = new QOpenGLVertexArrayObject();
    vao->create();
    vao->bind();

    QOpenGLBuffer* vbo;

    // Position buffer
    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo->allocate(vertices.data(), vertices.length() * sizeof(float));
    gl->glEnableVertexAttribArray((int)VertexUsage::Position);
    gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT, GL_FALSE,
                              3 * sizeof(float), BUFFER_OFFSET(0));

    // Normal buffer
    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo->allocate(normals.data(), normals.length() * sizeof(float));
    gl->glEnableVertexAttribArray((int)VertexUsage::Normal);
    gl->glVertexAttribPointer((int)VertexUsage::Normal, 3, GL_FLOAT, GL_FALSE,
                              3 * sizeof(float), BUFFER_OFFSET(0));

    // UV buffer
    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo->allocate(uvs.data(), uvs.length() * sizeof(float));
    gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
    gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT,
                              GL_FALSE, 2 * sizeof(float), BUFFER_OFFSET(0));

    // Tangent buffer
    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo->allocate(tangents.data(), tangents.length() * sizeof(float));
    gl->glEnableVertexAttribArray((int)VertexUsage::Tangent);
    gl->glVertexAttribPointer((int)VertexUsage::Tangent, 4, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), BUFFER_OFFSET(0));

    vao->release();

    // Index buffer
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
