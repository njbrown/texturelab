#include "../renderer/renderer.h"
#include "geometry.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <algorithm>
#include <vector>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

Mesh* createCube(QOpenGLFunctions* gl, float width, float height, float depth,
                 int widthSegments, int heightSegments, int depthSegments)
{
    widthSegments  = std::max(1, widthSegments);
    heightSegments = std::max(1, heightSegments);
    depthSegments  = std::max(1, depthSegments);

    // Interleaved layout per vertex: pos(3) normal(3) uv(2) tangent(4) = 12 floats
    static constexpr int kFloatsPerVertex = 12;
    static constexpr int kStride          = kFloatsPerVertex * sizeof(float);

    const int wS = widthSegments, hS = heightSegments, dS = depthSegments;
    const int totalVertices = 2 * ((dS+1)*(hS+1) + (wS+1)*(dS+1) + (wS+1)*(hS+1));
    const int totalIndices  = 12 * (dS*hS + wS*dS + wS*hS);

    std::vector<float>        interleaved;
    std::vector<unsigned int> indices;
    interleaved.reserve(totalVertices * kFloatsPerVertex);
    indices.reserve(totalIndices);

    int vertexOffset = 0;

    // u, v, w are axis indices (0=x,1=y,2=z) that map the face's local axes
    // onto world space. udir/vdir flip the winding. depth is the face offset.
    auto buildPlane = [&](int u, int v, int w, int udir, int vdir,
                          float faceW, float faceH, float faceD,
                          int gridX, int gridY)
    {
        const float segW    = faceW / gridX;
        const float segH    = faceH / gridY;
        const float halfW   = faceW / 2.0f;
        const float halfH   = faceH / 2.0f;
        const float halfD   = faceD / 2.0f;
        const int   gridX1  = gridX + 1;
        const int   gridY1  = gridY + 1;

        // Normal and tangent are constant across the face — compute once.
        float norm[3] = {};
        norm[w] = faceD > 0.0f ? 1.0f : -1.0f;

        float tang[3] = {};
        tang[u] = (float)udir;

        for (int iy = 0; iy < gridY1; iy++) {
            const float y = iy * segH - halfH;

            for (int ix = 0; ix < gridX1; ix++) {
                const float x = ix * segW - halfW;

                // position
                float pos[3] = {};
                pos[u] = x * udir;
                pos[v] = y * vdir;
                pos[w] = halfD;
                interleaved.push_back(pos[0]);
                interleaved.push_back(pos[1]);
                interleaved.push_back(pos[2]);

                // normal
                interleaved.push_back(norm[0]);
                interleaved.push_back(norm[1]);
                interleaved.push_back(norm[2]);

                // uv
                interleaved.push_back(ix / (float)gridX);
                interleaved.push_back(1.0f - (iy / (float)gridY));

                // tangent
                interleaved.push_back(tang[0]);
                interleaved.push_back(tang[1]);
                interleaved.push_back(tang[2]);
                interleaved.push_back(1.0f);
            }
        }

        for (int iy = 0; iy < gridY; iy++) {
            for (int ix = 0; ix < gridX; ix++) {
                const unsigned int a = vertexOffset + ix       + gridX1 * iy;
                const unsigned int b = vertexOffset + ix       + gridX1 * (iy + 1);
                const unsigned int c = vertexOffset + (ix + 1) + gridX1 * (iy + 1);
                const unsigned int d = vertexOffset + (ix + 1) + gridX1 * iy;

                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(d);

                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(d);
            }
        }

        vertexOffset += gridX1 * gridY1;
    };

    buildPlane(2, 1, 0, -1, -1, depth,  height,  width,  depthSegments,  heightSegments); // px
    buildPlane(2, 1, 0,  1, -1, depth,  height, -width,  depthSegments,  heightSegments); // nx
    buildPlane(0, 2, 1,  1,  1, width,  depth,   height, widthSegments,  depthSegments);  // py
    buildPlane(0, 2, 1,  1, -1, width,  depth,  -height, widthSegments,  depthSegments);  // ny
    buildPlane(0, 1, 2,  1, -1, width,  height,  depth,  widthSegments,  heightSegments); // pz
    buildPlane(0, 1, 2, -1, -1, width,  height, -depth,  widthSegments,  heightSegments); // nz

    QOpenGLVertexArrayObject* vao = new QOpenGLVertexArrayObject();
    vao->create();
    vao->bind();

    auto vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo->allocate(interleaved.data(), (int)(interleaved.size() * sizeof(float)));

    gl->glEnableVertexAttribArray((int)VertexUsage::Position);
    gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT, GL_FALSE,
                              kStride, BUFFER_OFFSET(0));

    gl->glEnableVertexAttribArray((int)VertexUsage::Normal);
    gl->glVertexAttribPointer((int)VertexUsage::Normal, 3, GL_FLOAT, GL_FALSE,
                              kStride, BUFFER_OFFSET(3 * sizeof(float)));

    gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
    gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT, GL_FALSE,
                              kStride, BUFFER_OFFSET(6 * sizeof(float)));

    gl->glEnableVertexAttribArray((int)VertexUsage::Tangent);
    gl->glVertexAttribPointer((int)VertexUsage::Tangent, 4, GL_FLOAT, GL_FALSE,
                              kStride, BUFFER_OFFSET(8 * sizeof(float)));

    vao->release();

    auto ibo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ibo->create();
    ibo->bind();
    ibo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    ibo->allocate(indices.data(), (int)(indices.size() * sizeof(unsigned int)));

    auto mesh = new Mesh();
    mesh->vao             = vao;
    mesh->meshType        = MeshType::Generated;
    mesh->indexBuffer     = ibo;
    mesh->numElements     = (int)indices.size();
    mesh->indexByteOffset = 0;
    mesh->indexType       = GL_UNSIGNED_INT;
    mesh->primitiveMode   = GL_TRIANGLES;

    return mesh;
}
