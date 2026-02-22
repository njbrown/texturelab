#include "../renderer/renderer.h"
#include "geometry.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <algorithm>
#include <math.h>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

Mesh* createCube(QOpenGLFunctions* gl, float width, float height, float depth,
                 int widthSegments, int heightSegments, int depthSegments)
{
    widthSegments = std::max(1, widthSegments);
    heightSegments = std::max(1, heightSegments);
    depthSegments = std::max(1, depthSegments);

    // buffers
    QVector<unsigned int> indices;
    QVector<float> vertices;
    QVector<float> normals;
    QVector<float> tangents;
    QVector<float> uvs;

    int vertexCount = 0;

    auto buildPlane = [&](int u, int v, int w, int udir, int vdir, float width,
                          float height, float depth, int gridX, int gridY) {
        float segmentWidth = width / gridX;
        float segmentHeight = height / gridY;

        float widthHalf = width / 2.0f;
        float heightHalf = height / 2.0f;
        float depthHalf = depth / 2.0f;

        int gridX1 = gridX + 1;
        int gridY1 = gridY + 1;

        int offset = vertexCount;

        QVector3D vec;

        // Generate vertices
        for (int iy = 0; iy < gridY1; iy++) {
            float y = iy * segmentHeight - heightHalf;

            for (int ix = 0; ix < gridX1; ix++) {
                float x = ix * segmentWidth - widthHalf;

                // Set vertex position
                vec[u] = x * udir;
                vec[v] = y * vdir;
                vec[w] = depthHalf;

                vertices.append(vec.x());
                vertices.append(vec.y());
                vertices.append(vec.z());

                // Set normal
                vec[u] = 0;
                vec[v] = 0;
                vec[w] = depth > 0 ? 1 : -1;

                normals.append(vec.x());
                normals.append(vec.y());
                normals.append(vec.z());

                // Set tangent
                QVector3D tangentVec;
                tangentVec[u] = udir;
                tangentVec[v] = 0;
                tangentVec[w] = 0;

                tangents.append(tangentVec.x());
                tangents.append(tangentVec.y());
                tangents.append(tangentVec.z());
                tangents.append(1.0f);

                // Set UV
                uvs.append(ix / (float)gridX);
                uvs.append(1.0f - (iy / (float)gridY));

                vertexCount++;
            }
        }

        // Generate indices
        for (int iy = 0; iy < gridY; iy++) {
            for (int ix = 0; ix < gridX; ix++) {
                unsigned int a = offset + ix + gridX1 * iy;
                unsigned int b = offset + ix + gridX1 * (iy + 1);
                unsigned int c = offset + (ix + 1) + gridX1 * (iy + 1);
                unsigned int d = offset + (ix + 1) + gridX1 * iy;

                // Two triangles per quad
                indices.append(a);
                indices.append(b);
                indices.append(d);

                indices.append(b);
                indices.append(c);
                indices.append(d);
            }
        }
    };

    // Build all 6 faces of the cube
    buildPlane(2, 1, 0, -1, -1, depth, height, width, depthSegments,
               heightSegments); // px
    buildPlane(2, 1, 0, 1, -1, depth, height, -width, depthSegments,
               heightSegments); // nx
    buildPlane(0, 2, 1, 1, 1, width, depth, height, widthSegments,
               depthSegments); // py
    buildPlane(0, 2, 1, 1, -1, width, depth, -height, widthSegments,
               depthSegments); // ny
    buildPlane(0, 1, 2, 1, -1, width, height, depth, widthSegments,
               heightSegments); // pz
    buildPlane(0, 1, 2, -1, -1, width, height, -depth, widthSegments,
               heightSegments); // nz

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
