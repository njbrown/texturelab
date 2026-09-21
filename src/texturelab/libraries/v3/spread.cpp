#include "../../graphics/noderenderer.h"
#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

#include <QOpenGLFunctions_3_2_Core>
#include <algorithm>

// Color Spread — JFA nearest-seed lookup that samples a color map instead of
// computing a distance value.  Each masked-out (black) pixel receives the exact
// color of the closest foreground pixel in the mask, producing solid filled
// regions with no gradient blending.
//
// Use case: flood-fill a pattern with random intensities, then spread those
// colors outward to cover the lines/gaps in the pattern.

// ============================================================================
// ColorSpreadRenderData
// ============================================================================
struct ColorSpreadRenderData : public NodeRenderData {
    float threshold = 0.5f;
    float distance = 1.0f;
};

// ============================================================================
// ColorSpreadRenderer
// ============================================================================
class ColorSpreadRenderer : public NodeTextureRenderer {
public:
    void render(NodeRenderContext& ctx, const NodeRenderData& baseData) override
    {
        auto& data = static_cast<const ColorSpreadRenderData&>(baseData);
        auto gl = ctx.gl;
        auto cache = ctx.cache;
        int w = ctx.textureWidth;
        int h = ctx.textureHeight;

        // Require at least the mask input; color map is optional (falls back to
        // mask)
        if (ctx.inputs.isEmpty() || ctx.inputs[0].textureId == 0) {
            cache->bindFboToTexture(ctx.outputTextureId);
            gl->glViewport(0, 0, w, h);
            gl->glClearColor(0, 0, 0, 1);
            gl->glClear(GL_COLOR_BUFFER_BIT);
            return;
        }

        GLuint maskTex = ctx.inputs[0].textureId;
        GLuint colorTex =
            (ctx.inputs.size() > 1 && ctx.inputs[1].textureId != 0)
                ? ctx.inputs[1].textureId
                : maskTex;

        GLuint seedShader = cache->getOrCompileShader(
            "cs_seed", RenderResourceCache::standardVertexSource(), seedFrag());
        GLuint jfaShader = cache->getOrCompileShader(
            "cs_jfa", RenderResourceCache::standardVertexSource(), jfaFrag());
        GLuint sampleShader = cache->getOrCompileShader(
            "cs_sample", RenderResourceCache::standardVertexSource(),
            sampleFrag());

        GLuint texA = cache->acquireTexture(w, h);
        GLuint texB = cache->acquireTexture(w, h);

        // Seed pass: foreground pixels (mask >= threshold) write their UV;
        // background pixels write the sentinel (-1, -1).
        cache->bindFboToTexture(texA);
        ctx.useShader(seedShader);
        ctx.bindTexture(seedShader, "u_mask", maskTex, 0);
        gl->glUniform1f(gl->glGetUniformLocation(seedShader, "u_threshold"),
                        data.threshold);
        ctx.drawQuad();

        // JFA passes: propagate nearest-seed UV across the texture
        int maxDim = std::max(w, h);
        int stepSize = maxDim / 2;
        while (stepSize >= 1) {
            cache->bindFboToTexture(texB);
            ctx.useShader(jfaShader);
            ctx.bindTexture(jfaShader, "u_input", texA, 0);
            gl->glUniform1i(gl->glGetUniformLocation(jfaShader, "u_step"),
                            stepSize);
            ctx.drawQuad();
            std::swap(texA, texB);
            stepSize /= 2;
        }

        // Sample pass: look up the color map at the nearest-seed UV
        cache->bindFboToTexture(ctx.outputTextureId);
        ctx.useShader(sampleShader);
        ctx.bindTexture(sampleShader, "u_jfa", texA, 0);
        ctx.bindTexture(sampleShader, "u_color", colorTex, 1);
        gl->glUniform1f(gl->glGetUniformLocation(sampleShader, "u_distance"),
                        data.distance);
        ctx.drawQuad();

        cache->releaseTexture(texA);
        cache->releaseTexture(texB);
    }

private:
    static QString seedFrag()
    {
        return R""""(
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_mask;
            uniform float     u_threshold;

            void main()
            {
                float v = texture(u_mask, v_texCoord).r;
                if (v >= u_threshold)
                    fragColor = vec4(v_texCoord, 0.0, 1.0);
                else
                    fragColor = vec4(-1.0, -1.0, 0.0, 1.0);
            }
        )"""";
    }

    static QString jfaFrag()
    {
        return R""""(
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_input;
            uniform int       u_step;

            void main()
            {
                vec2 texSize = vec2(textureSize(u_input, 0));
                vec2 step    = vec2(float(u_step)) / texSize;

                vec2  bestUV   = vec2(-1.0);
                float bestDist = 1e9;

                for (int x = -1; x <= 1; x++) {
                    for (int y = -1; y <= 1; y++) {
                        vec2 sampleUV = v_texCoord + vec2(float(x), float(y)) * step;
                        vec4 s        = texture(u_input, sampleUV);
                        vec2 seedUV   = s.rg;
                        if (seedUV.x < 0.0) continue;
                        float d = length(v_texCoord - seedUV);
                        if (d < bestDist) {
                            bestDist = d;
                            bestUV   = seedUV;
                        }
                    }
                }

                fragColor = vec4(bestUV, 0.0, 1.0);
            }
        )"""";
    }

    // Sample the color map at the nearest-seed UV for a solid, gradient-free
    // fill
    static QString sampleFrag()
    {
        return R""""(
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_jfa;
            uniform sampler2D u_color;
            uniform float     u_distance;

            void main()
            {
                vec2 seedUV = texture(u_jfa, v_texCoord).rg;

                if (seedUV.x < 0.0) {
                    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
                } else {
                    float d = length(v_texCoord - seedUV);
                    if (d > u_distance * 0.5)
                        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
                    else
                        fragColor = texture(u_color, seedUV);
                }
            }
        )"""";
    }
};

// ============================================================================
// ColorSpreadNode
// ============================================================================
void ColorSpreadNode::init()
{
    this->title = "Spread";

    this->addInput("color");
    this->addInput("mask");

    this->addFloatProp("threshold", "Threshold", 0.5, 0.0, 1.0, 0.01);
    this->addFloatProp("distance", "Distance", 1.0, 0.0, 1.0, 0.01);
}

std::shared_ptr<NodeTextureRenderer> ColorSpreadNode::createRenderer()
{
    return std::make_shared<ColorSpreadRenderer>();
}

std::shared_ptr<NodeRenderData> ColorSpreadNode::createRenderData()
{
    auto data = std::make_shared<ColorSpreadRenderData>();

    if (auto* p = dynamic_cast<FloatProp*>(getProp("threshold")))
        data->threshold = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<FloatProp*>(getProp("distance")))
        data->distance = static_cast<float>(p->value);

    return data;
}
