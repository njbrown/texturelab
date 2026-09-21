#include "../../graphics/noderenderer.h"
#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

#include <QOpenGLFunctions_3_2_Core>
#include <algorithm>
#include <cmath>

// Rong & Tan, "Jump Flooding in GPU with Applications to Voronoi Diagram and
//   Distance Transform," I3D 2006
// https://bgolus.medium.com/the-quest-for-very-wide-outlines-ba82ed442cd9
//   (Ben Golus — GPU distance transforms, JFA accuracy analysis)
//
// Jump Flooding Algorithm (JFA) Euclidean distance transform.
// Each foreground pixel in the thresholded mask seeds the JFA; subsequent
// passes propagate nearest-seed UVs across the texture in O(log N) passes.
// The final pass converts nearest-seed UV to a normalised distance value.
//
// Reuses the same JFA infrastructure as BevelV2; this node exposes the raw
// distance field rather than a bevel profile, making it a universal primitive
// for soft edge halos, wear gradients and stencil masks.

// ============================================================================
// DistanceTransformRenderData
// ============================================================================
struct DistanceTransformRenderData : public NodeRenderData {
    float threshold = 0.5f;
    float spread    = 0.5f;
    bool  invert    = false;
};

// ============================================================================
// DistanceTransformRenderer
// ============================================================================
class DistanceTransformRenderer : public NodeTextureRenderer {
public:
    void render(NodeRenderContext& ctx, const NodeRenderData& baseData) override
    {
        auto& data  = static_cast<const DistanceTransformRenderData&>(baseData);
        auto  gl    = ctx.gl;
        auto  cache = ctx.cache;
        int   w = ctx.textureWidth;
        int   h = ctx.textureHeight;

        if (ctx.inputs.isEmpty() || ctx.inputs[0].textureId == 0) {
            cache->bindFboToTexture(ctx.outputTextureId);
            gl->glViewport(0, 0, w, h);
            gl->glClearColor(0, 0, 0, 1);
            gl->glClear(GL_COLOR_BUFFER_BIT);
            return;
        }

        GLuint seedShader = cache->getOrCompileShader(
            "jfadt_seed",  RenderResourceCache::standardVertexSource(), seedFrag());
        GLuint jfaShader  = cache->getOrCompileShader(
            "jfadt_step",  RenderResourceCache::standardVertexSource(), jfaFrag());
        GLuint distShader = cache->getOrCompileShader(
            "jfadt_dist",  RenderResourceCache::standardVertexSource(), distFrag());

        GLuint texA = cache->acquireTexture(w, h);
        GLuint texB = cache->acquireTexture(w, h);

        // --- Seed pass: foreground pixels write their own UV; background writes (-1,-1) ---
        cache->bindFboToTexture(texA);
        ctx.useShader(seedShader);
        ctx.bindTexture(seedShader, "u_mask", ctx.inputs[0].textureId, 0);
        gl->glUniform1f(
            gl->glGetUniformLocation(seedShader, "u_threshold"),
            data.threshold);
        ctx.drawQuad();

        // --- JFA passes ---
        int maxDim   = std::max(w, h);
        int stepSize = maxDim / 2;
        while (stepSize >= 1) {
            cache->bindFboToTexture(texB);
            ctx.useShader(jfaShader);
            ctx.bindTexture(jfaShader, "u_input", texA, 0);
            gl->glUniform1i(
                gl->glGetUniformLocation(jfaShader, "u_step"), stepSize);
            ctx.drawQuad();
            std::swap(texA, texB);
            stepSize /= 2;
        }

        // --- Distance pass: convert nearest-seed UV to normalised distance ---
        cache->bindFboToTexture(ctx.outputTextureId);
        ctx.useShader(distShader);
        ctx.bindTexture(distShader, "u_jfa", texA, 0);
        gl->glUniform1f(
            gl->glGetUniformLocation(distShader, "u_spread"),
            data.spread);
        gl->glUniform1i(
            gl->glGetUniformLocation(distShader, "u_invert"),
            data.invert ? 1 : 0);
        ctx.drawQuad();

        cache->releaseTexture(texA);
        cache->releaseTexture(texB);
    }

private:
    // Seed pass: store UV in RG if foreground, sentinel (-1,-1) if background
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
                    fragColor = vec4(v_texCoord, 0.0, 1.0); // seed: own UV
                else
                    fragColor = vec4(-1.0, -1.0, 0.0, 1.0); // no seed
            }
        )"""";
    }

    // JFA step: for each pixel, sample 8 neighbours at ±stepSize and keep
    // whichever carries the nearest valid seed UV
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
                        vec2  sampleUV   = v_texCoord + vec2(float(x), float(y)) * step;
                        vec4  s          = texture(u_input, sampleUV);
                        vec2  seedUV     = s.rg;
                        if (seedUV.x < 0.0) continue; // no seed stored here
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

    // Distance pass: convert nearest-seed UV to a normalised, spread-scaled
    // grayscale value
    static QString distFrag()
    {
        return R""""(
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_jfa;
            uniform float     u_spread;
            uniform int       u_invert;

            void main()
            {
                vec2 seedUV = texture(u_jfa, v_texCoord).rg;

                float dist;
                if (seedUV.x < 0.0) {
                    // No seed found — maximum distance
                    dist = 1.0;
                } else {
                    // Raw distance in [0,~1.41] UV space
                    dist = length(v_texCoord - seedUV);
                    // Scale by spread: spread=0.5 → moderate falloff
                    float invSpread = 1.0 / max(u_spread, 0.001);
                    dist = clamp(dist * invSpread * 2.0, 0.0, 1.0);
                }

                if (u_invert == 0)
                    dist = 1.0 - dist; // bright near seed, dark far away (default)

                fragColor = vec4(vec3(dist), 1.0);
            }
        )"""";
    }
};

// ============================================================================
// DistanceTransformNode
// ============================================================================
void DistanceTransformNode::init()
{
    this->title = "Distance Transform";

    this->addInput("mask");

    this->addFloatProp("threshold", "Threshold", 0.5, 0.0, 1.0, 0.01);
    this->addFloatProp("spread",    "Spread",    0.5, 0.0, 1.0, 0.01);
    this->addBoolProp ("invert",    "Invert",    false);
}

std::shared_ptr<NodeTextureRenderer> DistanceTransformNode::createRenderer()
{
    return std::make_shared<DistanceTransformRenderer>();
}

std::shared_ptr<NodeRenderData> DistanceTransformNode::createRenderData()
{
    auto data = std::make_shared<DistanceTransformRenderData>();

    if (auto* p = dynamic_cast<FloatProp*>(getProp("threshold")))
        data->threshold = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<FloatProp*>(getProp("spread")))
        data->spread = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<BoolProp*>(getProp("invert")))
        data->invert = p->value;

    return data;
}
