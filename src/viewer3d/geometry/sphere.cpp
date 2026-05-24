#include "../renderer/renderer.h"
#include "geometry.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <algorithm>
#include <cmath>
#include <vector>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

Mesh* createSphere(QOpenGLFunctions* gl, float radius, int widthSegments,
                   int heightSegments, float phiStart, float phiLength,
                   float thetaStart, float thetaLength)
{
    const float uvScaleX = 2.0f;
    const float uvScaleY = 1.0f;

    widthSegments  = std::max(3, widthSegments);
    heightSegments = std::max(2, heightSegments);

    const double thetaEnd = std::min((double)thetaStart + thetaLength, M_PI);

    const int ringCount   = heightSegments + 1;
    const int colCount    = widthSegments  + 1;
    const int vertexCount = ringCount * colCount;

    // Precompute trig per column (phi) and per ring (theta) to avoid
    // redundant sin/cos calls inside the double loop.
    std::vector<float> sinPhi(colCount), cosPhi(colCount);
    for (int ix = 0; ix < colCount; ix++) {
        float phi  = phiStart + (ix / (float)widthSegments) * phiLength;
        sinPhi[ix] = std::sin(phi);
        cosPhi[ix] = std::cos(phi);
    }
    std::vector<float> sinTheta(ringCount), cosTheta(ringCount);
    for (int iy = 0; iy < ringCount; iy++) {
        float theta    = thetaStart + (iy / (float)heightSegments) * thetaLength;
        sinTheta[iy]   = std::sin(theta);
        cosTheta[iy]   = std::cos(theta);
    }

    // Interleaved layout per vertex: pos(3) normal(3) uv(2) tangent(4) = 12 floats
    static constexpr int kFloatsPerVertex = 12;
    static constexpr int kStride          = kFloatsPerVertex * sizeof(float);

    std::vector<float> interleaved;
    interleaved.reserve(vertexCount * kFloatsPerVertex);

    // Upper-bound index count: 6 per quad
    std::vector<unsigned int> indices;
    indices.reserve(widthSegments * heightSegments * 6);

    for (int iy = 0; iy < ringCount; iy++) {
        const float v    = iy / (float)heightSegments;
        const float sinT = sinTheta[iy];
        const float cosT = cosTheta[iy];

        float uOffset = 0.0f;
        if (iy == 0 && thetaStart == 0.0f)
            uOffset =  0.5f / widthSegments;
        else if (iy == heightSegments && thetaEnd == M_PI)
            uOffset = -0.5f / widthSegments;

        for (int ix = 0; ix < colCount; ix++) {
            const float u  = ix / (float)widthSegments;
            const float sP = sinPhi[ix];
            const float cP = cosPhi[ix];

            // Position
            const float x = -radius * cP * sinT;
            const float y =  radius * cosT;
            const float z =  radius * sP * sinT;

            interleaved.push_back(x);
            interleaved.push_back(y);
            interleaved.push_back(z);

            // Normal = position / radius (already unit length for a sphere)
            interleaved.push_back(x / radius);
            interleaved.push_back(y / radius);
            interleaved.push_back(z / radius);

            // UV
            interleaved.push_back((u + uOffset) * uvScaleX);
            interleaved.push_back((1.0f - v) * uvScaleY);

            // Tangent = d(pos)/d(phi) normalized = (sin(phi), 0, cos(phi), 1)
            // Already unit length: sqrt(sin²+cos²) = 1, no normalize needed.
            interleaved.push_back(sP);
            interleaved.push_back(0.0f);
            interleaved.push_back(cP);
            interleaved.push_back(1.0f);
        }
    }

    // Build indices with flat grid math — no 2D intermediate vector needed.
    for (int iy = 0; iy < heightSegments; iy++) {
        for (int ix = 0; ix < widthSegments; ix++) {
            const unsigned int a = iy       * colCount + ix + 1;
            const unsigned int b = iy       * colCount + ix;
            const unsigned int c = (iy + 1) * colCount + ix;
            const unsigned int d = (iy + 1) * colCount + ix + 1;

            if (iy != 0 || thetaStart > 0.0f) {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(d);
            }
            if (iy != heightSegments - 1 || thetaEnd < M_PI) {
                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(d);
            }
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
    mesh->vao           = vao;
    mesh->meshType      = MeshType::Generated;
    mesh->indexBuffer   = ibo;
    mesh->numElements   = (int)indices.size();
    mesh->indexByteOffset = 0;
    mesh->indexType     = GL_UNSIGNED_INT;
    mesh->primitiveMode = GL_TRIANGLES;

    return mesh;
}
