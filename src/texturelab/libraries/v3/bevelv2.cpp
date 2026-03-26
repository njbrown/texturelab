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
};

// ============================================================================
// BevelV2Renderer — JFA-based GPU bevel implementation
// ============================================================================

class BevelV2Renderer : public NodeTextureRenderer {
public:
    void render(NodeRenderContext& ctx,
                const NodeRenderData& baseData) override
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
        GLuint seedShader = cache->getOrCompileShader(
            "jfa_seed", standardVert(), seedFrag());
        GLuint jfaShader = cache->getOrCompileShader(
            "jfa_step", standardVert(), jfaFrag());
        GLuint bevelShader = cache->getOrCompileShader(
            "jfa_bevel", standardVert(), bevelFrag());

        // Acquire two intermediate textures for ping-pong
        GLuint texA = cache->acquireTexture(w, h);
        GLuint texB = cache->acquireTexture(w, h);

        // --- Pass 0: Seed initialization ---
        // Detect edges where the input crosses the threshold
        cache->bindFboToTexture(texA);
        ctx.useShader(seedShader);
        ctx.bindTexture(seedShader, "image", ctx.inputs[0].textureId, 0);
        gl->glUniform1f(
            gl->glGetUniformLocation(seedShader, "u_threshold"),
            data.threshold);
        ctx.drawQuad();

        // --- Passes 1..N: JFA iteration (ping-pong) ---
        int maxDim = std::max(w, h);
        int stepSize = maxDim / 2;

        while (stepSize >= 1) {
            cache->bindFboToTexture(texB);
            ctx.useShader(jfaShader);
            ctx.bindTexture(jfaShader, "u_input", texA, 0);
            gl->glUniform1i(
                gl->glGetUniformLocation(jfaShader, "u_stepSize"), stepSize);
            ctx.drawQuad();

            std::swap(texA, texB);
            stepSize /= 2;
        }

        // --- Final pass: Distance field → bevel height ---
        cache->bindFboToTexture(ctx.outputTextureId);
        ctx.useShader(bevelShader);
        ctx.bindTexture(bevelShader, "u_jfa", texA, 0);
        gl->glUniform1f(
            gl->glGetUniformLocation(bevelShader, "u_distance"),
            data.distance);
        ctx.drawQuad();

        // Intermediates released by worker after render() returns
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

            void main() {
                vec2 uv = v_texCoord;
                float v = texture(image, uv).r;

                // Black pixels (below threshold) are seeds —
                // JFA spreads distance from them into white regions
                if (v < u_threshold)
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

                float bevel = clamp(dist / u_distance, 0.0, 1.0);
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

    return data;
}
