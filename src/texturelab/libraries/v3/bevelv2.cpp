#include "../../graphics/noderenderer.h"
#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

#include <QOpenGLFunctions_3_2_Core>
#include <algorithm>
#include <cmath>

// ============================================================================
// BevelV2RenderData — parameters passed to the render thread
// ============================================================================

struct BevelV2RenderData : public NodeRenderData {
    float distance = 50.0f;
    float threshold = 0.5f;
    int shape = 0; // 0=Linear, 1=Round, 2=Smooth
    bool invert = false;
    bool scaleInvariant = true;
};

// ============================================================================
// BevelV2Renderer — JFA-based GPU bevel implementation
// ============================================================================

class BevelV2Renderer : public NodeTextureRenderer {
public:
    void render(NodeRenderContext& ctx, const NodeRenderData& baseData) override
    {
        auto& data = static_cast<const BevelV2RenderData&>(baseData);
        auto gl = ctx.gl;
        auto cache = ctx.cache;

        int w = ctx.textureWidth;
        int h = ctx.textureHeight;

        // No input connected — output black
        if (ctx.inputs.isEmpty() || ctx.inputs[0].textureId == 0) {
            cache->bindFboToTexture(ctx.outputTextureId);
            gl->glViewport(0, 0, w, h);
            gl->glClearColor(0, 0, 0, 1);
            gl->glClear(GL_COLOR_BUFFER_BIT);
            return;
        }

        // Compile shaders (cached after first call)
        GLuint seedShader =
            cache->getOrCompileShader("jfa_seed", standardVert(), seedFrag());
        GLuint jfaShader =
            cache->getOrCompileShader("jfa_step", standardVert(), jfaFrag());
        GLuint bevelShader =
            cache->getOrCompileShader("jfa_bevel", standardVert(), bevelFrag());

        // Acquire two intermediate textures for ping-pong
        GLuint texA = cache->acquireTexture(w, h);
        GLuint texB = cache->acquireTexture(w, h);

        // --- Seed initialization ---
        cache->bindFboToTexture(texA);
        ctx.useShader(seedShader);
        ctx.bindTexture(seedShader, "image", ctx.inputs[0].textureId, 0);
        gl->glUniform1f(gl->glGetUniformLocation(seedShader, "u_threshold"),
                        data.threshold);
        gl->glUniform1i(gl->glGetUniformLocation(seedShader, "u_invert"),
                        data.invert ? 1 : 0);
        ctx.drawQuad();

        // --- JFA iteration (ping-pong) ---
        int maxDim = std::max(w, h);
        int stepSize = maxDim / 2;

        while (stepSize >= 1) {
            cache->bindFboToTexture(texB);
            ctx.useShader(jfaShader);
            ctx.bindTexture(jfaShader, "u_input", texA, 0);
            gl->glUniform1i(gl->glGetUniformLocation(jfaShader, "u_stepSize"),
                            stepSize);
            ctx.drawQuad();

            std::swap(texA, texB);
            stepSize /= 2;
        }

        // --- Final pass: Distance field → bevel height ---
        cache->bindFboToTexture(ctx.outputTextureId);
        ctx.useShader(bevelShader);
        ctx.bindTexture(bevelShader, "u_jfa", texA, 0);
        gl->glUniform1f(gl->glGetUniformLocation(bevelShader, "u_distance"),
                        data.distance);
        gl->glUniform1i(gl->glGetUniformLocation(bevelShader, "u_shape"),
                        data.shape);
        gl->glUniform1i(gl->glGetUniformLocation(bevelShader, "u_invert"),
                        data.invert ? 1 : 0);
        gl->glUniform1i(
            gl->glGetUniformLocation(bevelShader, "u_scaleInvariant"),
            data.scaleInvariant ? 1 : 0);
        ctx.drawQuad();
    }

private:
    static QString standardVert()
    {
        return RenderResourceCache::standardVertexSource();
    }

    static QString seedFrag()
    {
        return R""""(
            #version 150 core
            in vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D image;
            uniform vec2 _textureSize;
            uniform float u_threshold;
            uniform int u_invert;

            void main() {
                vec2 uv = v_texCoord;
                float v = texture(image, uv).r;

                // Black pixels (below threshold) are seeds by default —
                // JFA spreads distance from them into white regions, so
                // white shapes end up raised and black background stays
                // flat. `invert` swaps which side is treated as the seed
                // (matching the old bevel node, whose two-sided signed
                // distance field meant flipping its single "invert" had
                // the effect of swapping which side the flat plateau fell
                // on, on top of flipping the output polarity below).
                bool isSeed = (u_invert == 1) ? (v >= u_threshold)
                                              : (v < u_threshold);

                if (isSeed)
                    fragColor = vec4(uv, v, 1.0);  // Seed: store own UV
                else
                    fragColor = vec4(-1.0, -1.0, v, 0.0);  // No seed
            }
        )"""";
    }

    static QString jfaFrag()
    {
        return R""""(
            #version 150 core
            in vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_input;
            uniform vec2 _textureSize;
            uniform int u_stepSize;

            void main() {
                vec2 uv = v_texCoord;
                vec2 texel = vec2(1.0) / _textureSize;
                vec4 best = texture(u_input, uv);
                float bestDist = (best.a < 0.5) ? 9999.0 : length(uv - best.xy);

                // Check 3x3 neighborhood at current step size
                for (int y = -1; y <= 1; y++) {
                    for (int x = -1; x <= 1; x++) {
                        if (x == 0 && y == 0) continue;

                        vec2 offset = vec2(float(x), float(y))
                                    * float(u_stepSize) * texel;
                        vec4 neighbor = texture(u_input, uv + offset);

                        if (neighbor.a < 0.5) continue;  // No seed

                        float d = length(uv - neighbor.xy);
                        if (d < bestDist) {
                            bestDist = d;
                            best = neighbor;
                        }
                    }
                }

                fragColor = best;
            }
        )"""";
    }

    static QString bevelFrag()
    {
        return R""""(
            #version 150 core
            in vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_jfa;
            uniform vec2 _textureSize;
            uniform float u_distance;
            uniform int u_shape;
            uniform int u_invert;
            uniform int u_scaleInvariant;

            #define SHAPE_LINEAR 0
            #define SHAPE_ROUND  1
            #define SHAPE_SMOOTH 2

            void main() {
                vec2 uv = v_texCoord;
                vec4 data = texture(u_jfa, uv);

                if (data.a < 0.5) {
                    // No nearest seed found
                    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
                    return;
                }

                // Convert UV-space distance to pixel distance
                float dist = length(uv - data.xy)
                           * max(_textureSize.x, _textureSize.y);

                // Scale-invariant: normalize against texture resolution so
                // the bevel width in UV space stays consistent across
                // resolutions (512 is the reference size the prop range
                // was tuned against). Disabled, `distance` is read as a
                // raw pixel count instead — matching the old (v1/v2) Bevel
                // node, which always operated in absolute pixel terms.
                float pixelDistance = (u_scaleInvariant == 1)
                    ? u_distance * (max(_textureSize.x, _textureSize.y) / 512.0)
                    : u_distance;
                float t = clamp(dist / pixelDistance, 0.0, 1.0);

                float bevel;
                if (u_shape == SHAPE_ROUND) {
                    // Circular cross-section: quarter-circle falloff
                    bevel = sqrt(1.0 - (1.0 - t) * (1.0 - t));
                } else if (u_shape == SHAPE_SMOOTH) {
                    // S-curve: smoothstep for gentle transitions
                    bevel = smoothstep(0.0, 1.0, t);
                } else {
                    // Linear ramp (default)
                    bevel = t;
                }

                // if (u_invert == 1)
                //     bevel = 1.0 - bevel;

                fragColor = vec4(vec3(bevel), 1.0);
            }
        )"""";
    }
};

// ============================================================================
// BevelV2Node
// ============================================================================

void BevelV2Node::init()
{
    this->title = "Bevel V2";
    this->addInput("image");
    this->addFloatProp("distance", "Distance", 50.0, 0.0, 200.0, 0.5);
    this->addFloatProp("threshold", "Threshold", 0.5, 0.0, 1.0, 0.01);
    this->addEnumProp("shape", "Shape", {"Linear", "Round", "Smooth"});
    this->addBoolProp("invert", "Invert", false);
    this->addBoolProp("scaleInvariant", "Scale Invariant", true);

    // Passthrough shader for initialization (not used during rendering —
    // custom renderer handles all passes)
    auto source = R""""(
        vec4 process(vec2 uv)
        {
            return texture(image, uv);
        }
    )"""";
    this->setShaderSource(source);
}

std::shared_ptr<NodeTextureRenderer> BevelV2Node::createRenderer()
{
    return std::make_shared<BevelV2Renderer>();
}

std::shared_ptr<NodeRenderData> BevelV2Node::createRenderData()
{
    auto data = std::make_shared<BevelV2RenderData>();

    auto distProp = static_cast<FloatProp*>(this->getProp("distance"));
    if (distProp)
        data->distance = distProp->value;

    auto threshProp = static_cast<FloatProp*>(this->getProp("threshold"));
    if (threshProp)
        data->threshold = threshProp->value;

    auto shapeProp = static_cast<EnumProp*>(this->getProp("shape"));
    if (shapeProp)
        data->shape = shapeProp->index;

    auto invertProp = static_cast<BoolProp*>(this->getProp("invert"));
    if (invertProp)
        data->invert = invertProp->value;

    auto scaleInvariantProp =
        static_cast<BoolProp*>(this->getProp("scaleInvariant"));
    if (scaleInvariantProp)
        data->scaleInvariant = scaleInvariantProp->value;

    return data;
}
