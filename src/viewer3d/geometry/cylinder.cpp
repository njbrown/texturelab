#include "../renderer/renderer.h"
#include "geometry.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <algorithm>
#include <cmath>
#include <vector>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

Mesh* createCylinder(QOpenGLFunctions* gl, float radiusTop, float radiusBottom,
                     float height, int radialSegments, int heightSegments,
                     bool openEnded)
{
    radialSegments = std::max(3, radialSegments);
    heightSegments = std::max(1, heightSegments);

    // Interleaved layout per vertex: pos(3) normal(3) uv(2) tangent(4) = 12 floats
    static constexpr int kFloatsPerVertex = 12;
    static constexpr int kStride          = kFloatsPerVertex * sizeof(float);

    const int colCount = radialSegments + 1;

    // Exact vertex / index counts for upfront reservation.
    const bool hasCaps = !openEnded;
    const bool hasTop  = hasCaps && radiusTop    > 0.0f;
    const bool hasBot  = hasCaps && radiusBottom > 0.0f;
    const int capVerts   = (hasTop ? colCount + 1 : 0) + (hasBot ? colCount + 1 : 0);
    const int capIndices = (hasTop ? radialSegments : 0) + (hasBot ? radialSegments : 0);

    const int totalVertices = (heightSegments + 1) * colCount + capVerts;
    const int totalIndices  = heightSegments * radialSegments * 6 + capIndices * 3;

    std::vector<float>        interleaved;
    std::vector<unsigned int> indices;
    interleaved.reserve(totalVertices * kFloatsPerVertex);
    indices.reserve(totalIndices);

    // Precompute sin/cos for each column — reused by torso and both caps.
    std::vector<float> sinT(colCount), cosT(colCount);
    for (int x = 0; x < colCount; x++) {
        const float theta = (x / (float)radialSegments) * (float)(M_PI * 2.0);
        sinT[x] = std::sin(theta);
        cosT[x] = std::cos(theta);
    }

    // Slope is constant for the whole cylinder/cone.
    const float slope     = std::atan2(radiusBottom - radiusTop, height);
    const float cosSlope  = std::cos(slope);
    const float sinSlope  = std::sin(slope);

    const float halfHeight = height / 2.0f;

    // -------------------------------------------------------------------------
    // Torso
    // -------------------------------------------------------------------------
    const int torsoVertexBase = 0;

    for (int y = 0; y <= heightSegments; y++) {
        const float v      = y / (float)heightSegments;
        const float radius = v * (radiusBottom - radiusTop) + radiusTop;
        const float vy     = -v * height + halfHeight;

        for (int x = 0; x < colCount; x++) {
            const float s = sinT[x], c = cosT[x];

            // position
            interleaved.push_back(radius * s);
            interleaved.push_back(vy);
            interleaved.push_back(radius * c);

            // normal — (sinTheta*cosSlope, sinSlope, cosTheta*cosSlope) is already unit length
            interleaved.push_back(s * cosSlope);
            interleaved.push_back(sinSlope);
            interleaved.push_back(c * cosSlope);

            // uv
            interleaved.push_back((x / (float)radialSegments) * 2.0f);
            interleaved.push_back(1.0f - v);

            // tangent — (cosTheta, 0, -sinTheta) is already unit length
            interleaved.push_back(c);
            interleaved.push_back(0.0f);
            interleaved.push_back(-s);
            interleaved.push_back(1.0f);
        }
    }

    for (int y = 0; y < heightSegments; y++) {
        for (int x = 0; x < radialSegments; x++) {
            const unsigned int a = torsoVertexBase + y       * colCount + x;
            const unsigned int b = torsoVertexBase + (y + 1) * colCount + x;
            const unsigned int c = torsoVertexBase + (y + 1) * colCount + x + 1;
            const unsigned int d = torsoVertexBase + y       * colCount + x + 1;

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(d);

            indices.push_back(b);
            indices.push_back(c);
            indices.push_back(d);
        }
    }

    int nextVertex = (heightSegments + 1) * colCount;

    // -------------------------------------------------------------------------
    // Top cap
    // -------------------------------------------------------------------------
    if (hasTop) {
        const unsigned int centerIndex = nextVertex++;

        // center
        interleaved.push_back(0.0f);
        interleaved.push_back(halfHeight);
        interleaved.push_back(0.0f);
        interleaved.push_back(0.0f); interleaved.push_back(-1.0f); interleaved.push_back(0.0f); // normal
        interleaved.push_back(0.5f); interleaved.push_back(0.5f);                                // uv
        interleaved.push_back(1.0f); interleaved.push_back(0.0f); interleaved.push_back(0.0f); interleaved.push_back(1.0f); // tangent

        for (int x = 0; x < colCount; x++) {
            const float s = sinT[x], c = cosT[x];

            interleaved.push_back(radiusTop * s);
            interleaved.push_back(halfHeight);
            interleaved.push_back(radiusTop * c);
            interleaved.push_back(0.0f); interleaved.push_back(-1.0f); interleaved.push_back(0.0f);
            interleaved.push_back((c * 0.5f) + 0.5f);
            interleaved.push_back((s * 0.5f) + 0.5f);
            interleaved.push_back(1.0f); interleaved.push_back(0.0f); interleaved.push_back(0.0f); interleaved.push_back(1.0f);

            nextVertex++;
        }

        for (int x = 0; x < radialSegments; x++) {
            indices.push_back(centerIndex + x + 2);
            indices.push_back(centerIndex + x + 1);
            indices.push_back(centerIndex);
        }
    }

    // -------------------------------------------------------------------------
    // Bottom cap
    // -------------------------------------------------------------------------
    if (hasBot) {
        const unsigned int centerIndex = nextVertex++;

        // center
        interleaved.push_back(0.0f);
        interleaved.push_back(-halfHeight);
        interleaved.push_back(0.0f);
        interleaved.push_back(0.0f); interleaved.push_back(1.0f); interleaved.push_back(0.0f); // normal
        interleaved.push_back(0.5f); interleaved.push_back(0.5f);                               // uv
        interleaved.push_back(1.0f); interleaved.push_back(0.0f); interleaved.push_back(0.0f); interleaved.push_back(1.0f); // tangent

        for (int x = 0; x < colCount; x++) {
            const float s = sinT[x], c = cosT[x];

            interleaved.push_back(radiusBottom * s);
            interleaved.push_back(-halfHeight);
            interleaved.push_back(radiusBottom * c);
            interleaved.push_back(0.0f); interleaved.push_back(1.0f); interleaved.push_back(0.0f);
            interleaved.push_back((c * 0.5f) + 0.5f);
            interleaved.push_back((s * 0.5f) + 0.5f);
            interleaved.push_back(1.0f); interleaved.push_back(0.0f); interleaved.push_back(0.0f); interleaved.push_back(1.0f);

            nextVertex++;
        }

        for (int x = 0; x < radialSegments; x++) {
            indices.push_back(centerIndex);
            indices.push_back(centerIndex + x + 1);
            indices.push_back(centerIndex + x + 2);
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
