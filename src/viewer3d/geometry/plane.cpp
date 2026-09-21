#include "../renderer/renderer.h"
#include "geometry.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <algorithm>
#include <vector>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

Mesh* createPlane(QOpenGLFunctions* gl, float width, float height,
                  int widthSegments, int heightSegments,
                  PlaneOrientation orientation)
{
    widthSegments  = std::max(1, widthSegments);
    heightSegments = std::max(1, heightSegments);

    const float width_half  = width  / 2.0f;
    const float height_half = height / 2.0f;

    const int gridX1 = widthSegments  + 1;
    const int gridY1 = heightSegments + 1;

    const float segment_width  = width  / widthSegments;
    const float segment_height = height / heightSegments;

    const int vertexCount = gridX1 * gridY1;

    // Interleaved layout per vertex: pos(3) normal(3) uv(2) tangent(4) = 12 floats
    static constexpr int kFloatsPerVertex = 12;
    static constexpr int kStride          = kFloatsPerVertex * sizeof(float);

    std::vector<float> interleaved;
    interleaved.reserve(vertexCount * kFloatsPerVertex);

    std::vector<unsigned int> indices;
    indices.reserve(widthSegments * heightSegments * 6);

    // Precompute orientation-dependent constants to keep the inner loop branch-free.
    float nx, ny, nz;
    float tx, ty, tz;
    const bool flipV = (orientation != PlaneOrientation::XZ);

    if (orientation == PlaneOrientation::XY) {
        nx = 0.0f; ny = 0.0f; nz = -1.0f;
        tx = 1.0f; ty = 0.0f; tz =  0.0f;
    } else if (orientation == PlaneOrientation::YZ) {
        nx = -1.0f; ny = 0.0f; nz = 0.0f;
        tx =  0.0f; ty = 0.0f; tz = 1.0f;
    } else { // XZ
        nx = 0.0f; ny = 1.0f; nz = 0.0f;
        tx = 1.0f; ty = 0.0f; tz = 0.0f;
    }

    for (int iy = 0; iy < gridY1; iy++) {
        const float v = iy * segment_height - height_half;

        for (int ix = 0; ix < gridX1; ix++) {
            const float u = ix * segment_width - width_half;

            // position
            if (orientation == PlaneOrientation::XY) {
                interleaved.push_back(u);
                interleaved.push_back(-v);
                interleaved.push_back(0.0f);
            } else if (orientation == PlaneOrientation::YZ) {
                interleaved.push_back(0.0f);
                interleaved.push_back(-v);
                interleaved.push_back(u);
            } else {
                interleaved.push_back(u);
                interleaved.push_back(0.0f);
                interleaved.push_back(v);
            }

            // normal
            interleaved.push_back(nx);
            interleaved.push_back(ny);
            interleaved.push_back(nz);

            // uv
            const float uvx = ix / (float)widthSegments;
            const float uvy = flipV ? 1.0f - (iy / (float)heightSegments)
                                    : iy / (float)heightSegments;
            interleaved.push_back(uvx);
            interleaved.push_back(uvy);

            // tangent
            interleaved.push_back(tx);
            interleaved.push_back(ty);
            interleaved.push_back(tz);
            interleaved.push_back(1.0f);
        }
    }

    for (int iy = 0; iy < heightSegments; iy++) {
        for (int ix = 0; ix < widthSegments; ix++) {
            const unsigned int a = ix       + gridX1 * iy;
            const unsigned int b = ix       + gridX1 * (iy + 1);
            const unsigned int c = (ix + 1) + gridX1 * (iy + 1);
            const unsigned int d = (ix + 1) + gridX1 * iy;

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(d);

            indices.push_back(b);
            indices.push_back(c);
            indices.push_back(d);
        }
    }

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
