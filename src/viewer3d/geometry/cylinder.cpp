#include "../renderer/renderer.h"
#include "geometry.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <algorithm>
#include <math.h>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

Mesh* createCylinder(QOpenGLFunctions* gl, float radiusTop, float radiusBottom,
                     float height, int radialSegments, int heightSegments,
                     bool openEnded)
{
    radialSegments = std::max(3, radialSegments);
    heightSegments = std::max(1, heightSegments);

    // buffers
    QVector<unsigned int> indices;
    QVector<float> vertices;
    QVector<float> normals;
    QVector<float> tangents;
    QVector<float> uvs;

    int index = 0;
    QVector<QVector<unsigned int>> indexArray;

    float halfHeight = height / 2.0f;

    // Generate torso
    for (int y = 0; y <= heightSegments; y++) {
        QVector<unsigned int> indexRow;

        float v = y / (float)heightSegments;
        float radius = v * (radiusBottom - radiusTop) + radiusTop;

        for (int x = 0; x <= radialSegments; x++) {
            float u = x / (float)radialSegments;
            float theta = u * M_PI * 2.0f;

            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            // Vertex position
            float vx = radius * sinTheta;
            float vy = -v * height + halfHeight;
            float vz = radius * cosTheta;

            vertices.append(vx);
            vertices.append(vy);
            vertices.append(vz);

            // Normal (accounting for cone slope)
            float slope = std::atan2(radiusBottom - radiusTop, height);
            QVector3D normal(sinTheta * std::cos(slope), std::sin(slope),
                             cosTheta * std::cos(slope));
            normal.normalize();
            normals.append(normal.x());
            normals.append(normal.y());
            normals.append(normal.z());

            // Tangent (perpendicular to normal, going around the cylinder)
            QVector3D tangent(cosTheta, 0.0f, -sinTheta);
            tangent.normalize();
            tangents.append(tangent.x());
            tangents.append(tangent.y());
            tangents.append(tangent.z());
            tangents.append(1.0f);

            // UV
            uvs.append(u * 2.0f);
            uvs.append(1.0f - v);

            indexRow.append(index++);
        }

        indexArray.append(indexRow);
    }

    // Generate indices for torso
    for (int y = 0; y < heightSegments; y++) {
        for (int x = 0; x < radialSegments; x++) {
            unsigned int a = indexArray[y][x];
            unsigned int b = indexArray[y + 1][x];
            unsigned int c = indexArray[y + 1][x + 1];
            unsigned int d = indexArray[y][x + 1];

            indices.append(a);
            indices.append(b);
            indices.append(d);

            indices.append(b);
            indices.append(c);
            indices.append(d);
        }
    }

    // Generate top cap
    if (!openEnded && radiusTop > 0) {
        unsigned int centerIndex = index;

        // Center vertex
        vertices.append(0.0f);
        vertices.append(halfHeight);
        vertices.append(0.0f);

        normals.append(0.0f);
        normals.append(-1.0f);
        normals.append(0.0f);

        tangents.append(1.0f);
        tangents.append(0.0f);
        tangents.append(0.0f);
        tangents.append(1.0f);

        uvs.append(0.5f);
        uvs.append(0.5f);

        index++;

        // Ring vertices
        for (int x = 0; x <= radialSegments; x++) {
            float u = x / (float)radialSegments;
            float theta = u * M_PI * 2.0f;

            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            vertices.append(radiusTop * sinTheta);
            vertices.append(halfHeight);
            vertices.append(radiusTop * cosTheta);

            normals.append(0.0f);
            normals.append(-1.0f);
            normals.append(0.0f);

            tangents.append(1.0f);
            tangents.append(0.0f);
            tangents.append(0.0f);
            tangents.append(1.0f);

            uvs.append((cosTheta * 0.5f) + 0.5f);
            uvs.append((sinTheta * 0.5f) + 0.5f);

            index++;
        }

        // Generate top cap indices
        for (int x = 0; x < radialSegments; x++) {
            unsigned int c = centerIndex + x + 1;
            unsigned int d = centerIndex + x + 2;

            indices.append(d);
            indices.append(c);
            indices.append(centerIndex);
        }
    }

    // Generate bottom cap
    if (!openEnded && radiusBottom > 0) {
        unsigned int centerIndex = index;

        // Center vertex
        vertices.append(0.0f);
        vertices.append(-halfHeight);
        vertices.append(0.0f);

        normals.append(0.0f);
        normals.append(1.0f);
        normals.append(0.0f);

        tangents.append(1.0f);
        tangents.append(0.0f);
        tangents.append(0.0f);
        tangents.append(1.0f);

        uvs.append(0.5f);
        uvs.append(0.5f);

        index++;

        // Ring vertices
        for (int x = 0; x <= radialSegments; x++) {
            float u = x / (float)radialSegments;
            float theta = u * M_PI * 2.0f;

            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            vertices.append(radiusBottom * sinTheta);
            vertices.append(-halfHeight);
            vertices.append(radiusBottom * cosTheta);

            normals.append(0.0f);
            normals.append(1.0f);
            normals.append(0.0f);

            tangents.append(1.0f);
            tangents.append(0.0f);
            tangents.append(0.0f);
            tangents.append(1.0f);

            uvs.append((cosTheta * 0.5f) + 0.5f);
            uvs.append((sinTheta * 0.5f) + 0.5f);

            index++;
        }

        // Generate bottom cap indices
        for (int x = 0; x < radialSegments; x++) {
            unsigned int c = centerIndex + x + 1;
            unsigned int d = centerIndex + x + 2;

            indices.append(centerIndex);
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
