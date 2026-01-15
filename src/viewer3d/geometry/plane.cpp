#include "../renderer/renderer.h"
#include "geometry.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <algorithm>
#include <math.h>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

Mesh* createPlane(QOpenGLFunctions* gl, float width, float height,
                  int widthSegments, int heightSegments,
                  PlaneOrientation orientation)
{
    widthSegments = std::max(1, widthSegments);
    heightSegments = std::max(1, heightSegments);

    float width_half = width / 2.0f;
    float height_half = height / 2.0f;

    int gridX = widthSegments;
    int gridY = heightSegments;

    int gridX1 = gridX + 1;
    int gridY1 = gridY + 1;

    float segment_width = width / gridX;
    float segment_height = height / gridY;

    // buffers
    QVector<unsigned int> indices;
    QVector<float> vertices;
    QVector<float> normals;
    QVector<float> tangents;
    QVector<float> uvs;

    // Generate vertices, normals, uvs
    for (int iy = 0; iy < gridY1; iy++) {
        float v = iy * segment_height - height_half;

        for (int ix = 0; ix < gridX1; ix++) {
            float u = ix * segment_width - width_half;

            if (orientation == PlaneOrientation::XY) {
                // XY plane, normal facing -Z
                vertices.append(u);
                vertices.append(-v);
                vertices.append(0.0f);

                normals.append(0.0f);
                normals.append(0.0f);
                normals.append(-1.0f);

                tangents.append(1.0f);
                tangents.append(0.0f);
                tangents.append(0.0f);
                tangents.append(1.0f);
            }
            else if (orientation == PlaneOrientation::YZ) {
                // YZ plane, normal facing -X
                vertices.append(0.0f);
                vertices.append(-v);
                vertices.append(u);

                normals.append(-1.0f);
                normals.append(0.0f);
                normals.append(0.0f);

                tangents.append(0.0f);
                tangents.append(0.0f);
                tangents.append(1.0f);
                tangents.append(1.0f);
            }
            else { // PlaneOrientation::XZ
                // XZ plane, normal facing +Y
                vertices.append(u);
                vertices.append(0.0f);
                vertices.append(v);

                normals.append(0.0f);
                normals.append(1.0f);
                normals.append(0.0f);

                tangents.append(1.0f);
                tangents.append(0.0f);
                tangents.append(0.0f);
                tangents.append(1.0f);
            }

            // UV coordinates
            uvs.append(ix / (float)gridX);
            if (orientation == PlaneOrientation::XZ) {
                uvs.append(iy / (float)gridY);
            } else {
                uvs.append(1.0f - (iy / (float)gridY));
            }
        }
    }

    // Generate indices
    for (int iy = 0; iy < gridY; iy++) {
        for (int ix = 0; ix < gridX; ix++) {
            unsigned int a = ix + gridX1 * iy;
            unsigned int b = ix + gridX1 * (iy + 1);
            unsigned int c = (ix + 1) + gridX1 * (iy + 1);
            unsigned int d = (ix + 1) + gridX1 * iy;

            // Two triangles per quad
            indices.append(a);
            indices.append(b);
            indices.append(d);

            indices.append(b);
            indices.append(c);
            indices.append(d);
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
