#include "noderenderer.h"
#include "../props.h"

#include <QDebug>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

// ============================================================================
// RenderResourceCache
// ============================================================================

void RenderResourceCache::init(QOpenGLFunctions_3_2_Core* glFuncs, GLuint fboId)
{
    gl = glFuncs;
    m_fboId = fboId;
}

void RenderResourceCache::cleanup()
{
    if (!gl)
        return;

    for (auto& tex : texturePool) {
        if (tex.id != 0)
            gl->glDeleteTextures(1, &tex.id);
    }
    texturePool.clear();

    qDeleteAll(shaderCache);
    shaderCache.clear();
}

void RenderResourceCache::applyDefaultTextureParams(
    QOpenGLFunctions_3_2_Core* gl, GLuint textureId)
{
    gl->glBindTexture(GL_TEXTURE_2D, textureId);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint RenderResourceCache::acquireTexture(int width, int height)
{
    // Reuse an existing free texture of matching size
    for (auto& tex : texturePool) {
        if (!tex.inUse && tex.width == width && tex.height == height) {
            tex.inUse = true;
            // Reset here rather than trusting the previous user to have put
            // things back: a renderer that returned early, or simply forgot,
            // would otherwise hand the next one LINEAR or REPEAT sampling and
            // produce a bug that only shows up in certain node orderings.
            applyDefaultTextureParams(gl, tex.id);
            return tex.id;
        }
    }

    // Create new texture
    GLuint texId;
    gl->glGenTextures(1, &texId);
    gl->glBindTexture(GL_TEXTURE_2D, texId);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
                     GL_FLOAT, nullptr);
    gl->glBindTexture(GL_TEXTURE_2D, 0);
    applyDefaultTextureParams(gl, texId);

    CachedTexture cached;
    cached.id = texId;
    cached.width = width;
    cached.height = height;
    cached.inUse = true;
    texturePool.append(cached);

    return texId;
}

void RenderResourceCache::releaseTexture(GLuint textureId)
{
    for (auto& tex : texturePool) {
        if (tex.id == textureId) {
            tex.inUse = false;
            return;
        }
    }
}

void RenderResourceCache::releaseAllTextures()
{
    for (auto& tex : texturePool) {
        tex.inUse = false;
    }
}

GLuint RenderResourceCache::getOrCompileShader(const QString& key,
                                               const QString& vertexSource,
                                               const QString& fragmentSource)
{
    if (shaderCache.contains(key))
        return shaderCache[key]->programId();

    auto program = new QOpenGLShaderProgram();

    if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
        qDebug() << "NodeRenderer: Vertex shader error [" << key << "]";
        qDebug() << program->log();
    }

    if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                          fragmentSource)) {
        qDebug() << "NodeRenderer: Fragment shader error [" << key << "]";
        qDebug() << program->log();
    }

    // Bind attribute locations matching the worker's VBO layout
    program->bindAttributeLocation("a_pos", 0);    // VertexUsage::Position
    program->bindAttributeLocation("a_color", 1);  // VertexUsage::Color
    program->bindAttributeLocation("a_texCoord", 2); // VertexUsage::TexCoord0

    if (!program->link()) {
        qDebug() << "NodeRenderer: Shader link error [" << key << "]";
        qDebug() << program->log();
    }

    shaderCache[key] = program;
    return program->programId();
}

GLuint RenderResourceCache::compileNodeShader(
    const QString& key, const QString& processSource,
    const QStringList& inputNames,
    const QList<QPair<QString, int>>& propTypes)
{
    if (shaderCache.contains(key))
        return shaderCache[key]->programId();

    QString fSource = fragmentPreamble() + randomLib() + gradientLib() +
                      curveLib() +
                      generateInputDeclarations(inputNames) +
                      generatePropDeclarations(propTypes) + "#line 0\n" +
                      processSource;

    return getOrCompileShader(key, standardVertexSource(), fSource);
}

void RenderResourceCache::bindFboToTexture(GLuint textureId)
{
    gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, textureId, 0);
}

// ============================================================================
// Static shader source helpers
// ============================================================================

QString RenderResourceCache::standardVertexSource()
{
    return R""""(
        #version 150 core
        in vec3 a_pos;
        in vec2 a_texCoord;
        out vec2 v_texCoord;
        void main() {
            v_texCoord = a_texCoord;
            gl_Position = vec4(a_pos, 1);
        }
    )"""";
}

QString RenderResourceCache::fragmentPreamble()
{
    return R""""(
        #version 150 core
        in vec2 v_texCoord;

        #define GRADIENT_MAX_POINTS 32
        #define CURVE_MAX_POINTS 32

        vec4 process(vec2 uv);
        void initRandom();

        uniform vec2 _textureSize;

        out vec4 fragColor;

        void main() {
            initRandom();
            vec4 result = process(v_texCoord);
            fragColor = clamp(result, 0.0, 1.0);
        }
    )"""";
}

QString RenderResourceCache::randomLib()
{
    // Exact copy of TextureRenderer::createRandomLib()
    return R""""(
        uniform float _seed;
        vec2 _randomStart;

        #define RANDOM_ITERATIONS 1

        #define HASHSCALE1 443.8975
        #define HASHSCALE3 vec3(443.897, 441.423, 437.195)
        #define HASHSCALE4 vec4(443.897, 441.423, 437.195, 444.129)

        float hash12(vec2 p)
        {
            vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
            p3 += dot(p3, p3.yzx + 19.19);
            return fract((p3.x + p3.y) * p3.z);
        }

        vec2 hash22(vec2 p)
        {
            vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
            p3 += dot(p3, p3.yzx+19.19);
            return fract((p3.xx+p3.yz)*p3.zy);
        }

        float _rand(vec2 uv)
        {
            float a = 0.0;
            for (int t = 0; t < RANDOM_ITERATIONS; t++)
            {
                float v = float(t+1)*.152;
                vec2 pos = (uv * v);
                a += hash12(pos);
            }
            return a/float(RANDOM_ITERATIONS);
        }

        vec2 _rand2(vec2 uv)
        {
            vec2 a = vec2(0.0);
            for (int t = 0; t < RANDOM_ITERATIONS; t++)
            {
                float v = float(t+1)*.152;
                vec2 pos = (uv * v);
                a += hash22(pos);
            }
            return a/float(RANDOM_ITERATIONS);
        }

        float randomFloat(int index)
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomVec2(int index)
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomFloat(int index, float start, float end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + r*(end-start);
        }

        int randomInt(int index, int start, int end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + int(r*float(end-start));
        }

        bool randomBool(int index)
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index)) > 0.5;
        }

        void initRandom()
        {
            _randomStart = v_texCoord;
        }
    )"""";
}

QString RenderResourceCache::gradientLib()
{
    // Exact copy of TextureRenderer::createGradientLib()
    return R""""(
        struct Gradient {
            vec3 colors[GRADIENT_MAX_POINTS];
            float positions[GRADIENT_MAX_POINTS];
            int numPoints;
        };

        vec3 sampleGradient(vec3 colors[GRADIENT_MAX_POINTS],
                            float positions[GRADIENT_MAX_POINTS],
                            int numPoints, float t)
        {
            if (numPoints == 0)
                return vec3(1,0,0);

            if (numPoints == 1)
                return colors[0];

            if (t <= positions[0])
                return colors[0];

            int last = numPoints - 1;
            if (t >= positions[last])
                return colors[last];

            for(int i = 0; i < numPoints-1; i++) {
                if (positions[i+1] > t) {
                    vec3 colorA = colors[i];
                    vec3 colorB = colors[i+1];

                    float t1 = positions[i];
                    float t2 = positions[i+1];

                    float lerpPos = (t - t1)/(t2 - t1);
                    return mix(colorA, colorB, lerpPos);
                }
            }

            return vec3(0,0,0);
        }

        vec3 sampleGradient(Gradient gradient, float t)
        {
            return sampleGradient(gradient.colors, gradient.positions,
                                  gradient.numPoints, t);
        }
    )"""";
}

QString RenderResourceCache::curveLib()
{
    return R""""(
        struct Curve {
            int  numPoints;
            vec2 anchors[CURVE_MAX_POINTS];
            vec2 handleR[CURVE_MAX_POINTS];
            vec2 handleL[CURVE_MAX_POINTS];
        };

        float _cubicBez(float p0, float p1, float p2, float p3, float t) {
            float mt = 1.0 - t;
            return mt*mt*mt*p0 + 3.0*mt*mt*t*p1 + 3.0*mt*t*t*p2 + t*t*t*p3;
        }

        float _cubicBezD(float p0, float p1, float p2, float p3, float t) {
            float mt = 1.0 - t;
            return 3.0 * (mt*mt*(p1-p0) + 2.0*mt*t*(p2-p1) + t*t*(p3-p2));
        }

        float evalCurve(Curve c, float x) {
            // Find segment: last anchor whose x <= input x
            int seg = c.numPoints - 2;
            for (int i = 0; i < CURVE_MAX_POINTS - 1; i++) {
                if (i >= c.numPoints - 1) break;
                if (x <= c.anchors[i + 1].x) { seg = i; break; }
            }

            vec2 P0 = c.anchors[seg];
            vec2 P1 = c.handleR[seg];
            vec2 P2 = c.handleL[seg + 1];
            vec2 P3 = c.anchors[seg + 1];

            // Newton-Raphson: solve Bx(t) = x (8 iterations, constant bound)
            float t = clamp((x - P0.x) / max(P3.x - P0.x, 1e-5), 0.0, 1.0);
            for (int i = 0; i < 8; i++) {
                float bx  = _cubicBez(P0.x, P1.x, P2.x, P3.x, t);
                float dbx = _cubicBezD(P0.x, P1.x, P2.x, P3.x, t);
                t -= (abs(dbx) > 1e-6) ? (bx - x) / dbx : 0.0;
                t  = clamp(t, 0.0, 1.0);
            }

            return _cubicBez(P0.y, P1.y, P2.y, P3.y, t);
        }
    )"""";
}

QString RenderResourceCache::generateInputDeclarations(
    const QStringList& inputNames)
{
    QString code;
    for (const auto& input : inputNames) {
        code += "uniform sampler2D " + input + ";\n";
        code += "uniform bool " + input + "_connected;\n";
    }
    return code;
}

QString RenderResourceCache::generatePropDeclarations(
    const QList<QPair<QString, int>>& propTypes)
{
    QString code;
    for (const auto& prop : propTypes) {
        const QString& name = prop.first;
        auto type = static_cast<PropType::Value>(prop.second);

        switch (type) {
        case PropType::Int:
            code += "uniform int prop_" + name + ";\n";
            break;
        case PropType::Float:
            code += "uniform float prop_" + name + ";\n";
            break;
        case PropType::Bool:
            code += "uniform bool prop_" + name + ";\n";
            break;
        case PropType::Enum:
            code += "uniform int prop_" + name + ";\n";
            break;
        case PropType::Color:
            code += "uniform vec4 prop_" + name + ";\n";
            break;
        case PropType::Gradient:
            code += "uniform Gradient prop_" + name + ";\n";
            break;
        case PropType::Image:
            code += "uniform sampler2D prop_" + name + ";\n";
            break;
        case PropType::Curve:
            code += "uniform Curve prop_" + name + ";\n";
            break;
        default:
            break;
        }
    }
    return code + "\n";
}

// ============================================================================
// ScopedTextureParams
// ============================================================================

ScopedTextureParams::ScopedTextureParams(QOpenGLFunctions_3_2_Core* glFuncs,
                                         GLuint textureId, GLint minFilter,
                                         GLint magFilter, GLint wrap)
    : gl(glFuncs), tex(textureId)
{
    gl->glBindTexture(GL_TEXTURE_2D, tex);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    gl->glBindTexture(GL_TEXTURE_2D, 0);
}

ScopedTextureParams::~ScopedTextureParams()
{
    RenderResourceCache::applyDefaultTextureParams(gl, tex);
}

// ============================================================================
// NodeRenderContext
// ============================================================================

void NodeRenderContext::useShader(GLuint programId)
{
    gl->glUseProgram(programId);
    gl->glViewport(0, 0, textureWidth, textureHeight);
    gl->glUniform2f(gl->glGetUniformLocation(programId, "_textureSize"),
                    (float)textureWidth, (float)textureHeight);
    gl->glUniform1f(gl->glGetUniformLocation(programId, "_seed"), randomSeed);
}

void NodeRenderContext::bindTexture(GLuint programId,
                                    const QString& uniformName,
                                    GLuint textureId, int unit)
{
    gl->glActiveTexture(GL_TEXTURE0 + unit);
    gl->glBindTexture(GL_TEXTURE_2D, textureId);
    gl->glUniform1i(
        gl->glGetUniformLocation(programId,
                                 uniformName.toStdString().c_str()),
        unit);
}

void NodeRenderContext::drawQuad()
{
    vao->bind();
    vbo->bind();

    // a_pos = attribute 0, a_texCoord = attribute 2
    gl->glEnableVertexAttribArray(0);
    gl->glEnableVertexAttribArray(2);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              nullptr);
    gl->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              reinterpret_cast<void*>(3 * sizeof(float)));

    gl->glDrawArrays(GL_TRIANGLES, 0, 6);

    vbo->release();
    vao->release();
}
