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
                     float bevelRadius, int bevelSegments,
                     float uvScaleU, float uvScaleV)
{
    radialSegments = std::max(3, radialSegments);
    heightSegments = std::max(1, heightSegments);
    bevelSegments  = std::max(1, bevelSegments);
    bevelRadius    = std::min({bevelRadius, radiusTop, radiusBottom, height / 2.0f});

    // Interleaved layout per vertex: pos(3) normal(3) uv(2) tangent(4) = 12 floats
    static constexpr int kFloatsPerVertex = 12;
    static constexpr int kStride          = kFloatsPerVertex * sizeof(float);

    // Ring layout top→bottom:
    //   top bevel:    bevelSegments+1 rings  (0 .. bevelSegments)
    //   torso:        heightSegments   rings  (bevelSegments+1 .. bevelSegments+heightSegments)
    //   bottom bevel: bevelSegments    rings  (bevelSegments+heightSegments+1 .. 2*bevelSegments+heightSegments)
    // Junction rings are owned by the preceding section; the next section skips ring 0.
    const int totalRings   = 2 * bevelSegments + heightSegments + 1;
    const int colCount     = radialSegments + 1;
    const int totalVertices = totalRings * colCount;
    const int totalIndices  = (totalRings - 1) * radialSegments * 6;

    std::vector<float>        interleaved;
    std::vector<unsigned int> indices;
    interleaved.reserve(totalVertices * kFloatsPerVertex);
    indices.reserve(totalIndices);

    // Precompute sin/cos per column — reused by all sections.
    std::vector<float> sinT(colCount), cosT(colCount);
    for (int x = 0; x < colCount; x++) {
        const float theta = (x / (float)radialSegments) * (float)(M_PI * 2.0);
        sinT[x] = std::sin(theta);
        cosT[x] = std::cos(theta);
    }

    const float halfHeight = height / 2.0f;

    // Slope for the torso normal (constant; 0 for a true cylinder).
    const float slope    = std::atan2(radiusBottom - radiusTop, height);
    const float cosSlope = std::cos(slope);
    const float sinSlope = std::sin(slope);

    // Push one full ring. normR = outward radial scale, normY = vertical normal component.
    int ringIndex = 0;
    auto pushRing = [&](float r, float y, float normR, float normY) {
        const float globalV = ringIndex / (float)(totalRings - 1);
        for (int x = 0; x < colCount; x++) {
            const float s = sinT[x], c = cosT[x];

            interleaved.push_back(r * s);
            interleaved.push_back(y);
            interleaved.push_back(r * c);

            interleaved.push_back(normR * s);
            interleaved.push_back(normY);
            interleaved.push_back(normR * c);

            interleaved.push_back((x / (float)radialSegments) * uvScaleU);
            interleaved.push_back((1.0f - globalV) * uvScaleV);

            interleaved.push_back(c);
            interleaved.push_back(0.0f);
            interleaved.push_back(-s);
            interleaved.push_back(1.0f);
        }
        ringIndex++;
    };

    // ---- Top bevel (rings 0..bevelSegments) ----
    // phi=PI/2 → top rim; phi=0 → torso junction.
    for (int i = 0; i <= bevelSegments; i++) {
        const float phi  = (float)(bevelSegments - i) / bevelSegments * (float)(M_PI / 2.0);
        const float r    = (radiusTop - bevelRadius) + bevelRadius * std::cos(phi);
        const float y    = (halfHeight - bevelRadius) + bevelRadius * std::sin(phi);
        pushRing(r, y, std::cos(phi), std::sin(phi));
    }

    // ---- Torso (rings bevelSegments+1..bevelSegments+heightSegments) ----
    // Skip j=0 — that junction ring was already pushed by the top bevel.
    for (int j = 1; j <= heightSegments; j++) {
        const float v = j / (float)heightSegments;
        const float r = v * (radiusBottom - radiusTop) + radiusTop;
        const float y = (halfHeight - bevelRadius) - v * (height - 2.0f * bevelRadius);
        pushRing(r, y, cosSlope, sinSlope);
    }

    // ---- Bottom bevel (rings bevelSegments+heightSegments+1..2*bevelSegments+heightSegments) ----
    // Skip k=0 — that junction ring was already pushed by the torso.
    // phi=0 → torso junction; phi=PI/2 → bottom rim.
    for (int k = 1; k <= bevelSegments; k++) {
        const float phi  = (float)k / bevelSegments * (float)(M_PI / 2.0);
        const float r    = (radiusBottom - bevelRadius) + bevelRadius * std::cos(phi);
        const float y    = -(halfHeight - bevelRadius) - bevelRadius * std::sin(phi);
        pushRing(r, y, std::cos(phi), -std::sin(phi));
    }

    // ---- Indices: connect every adjacent pair of rings ----
    for (int r = 0; r < totalRings - 1; r++) {
        for (int x = 0; x < radialSegments; x++) {
            const unsigned int a = r       * colCount + x;
            const unsigned int b = (r + 1) * colCount + x;
            const unsigned int c = (r + 1) * colCount + x + 1;
            const unsigned int d = r       * colCount + x + 1;

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
