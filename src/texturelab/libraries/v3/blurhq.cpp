#include "../../graphics/noderenderer.h"
#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

#include <QOpenGLFunctions_3_2_Core>
#include <algorithm>
#include <cmath>

// https://learnopengl.com/Advanced-Lighting/Bloom (separated Gaussian in GLSL)
// Wells, "Efficient Synthesis of Gaussian Filters by Cascaded Uniform Filters,"
//   IEEE PAMI 1986
// True separated Gaussian blur: two 1D passes (H then V) give O(2N) samples
// vs O(N²) for a 2D kernel — making large-radius blurs practical on the GPU.
// Use in preference to the V2 Blur node whenever quality matters.

// ============================================================================
// BlurHQRenderData
// ============================================================================
struct BlurHQRenderData : public NodeRenderData {
    float radius = 8.0f;
};

// ============================================================================
// BlurHQRenderer
// ============================================================================
class BlurHQRenderer : public NodeTextureRenderer {
public:
    void render(NodeRenderContext& ctx, const NodeRenderData& baseData) override
    {
        auto& data = static_cast<const BlurHQRenderData&>(baseData);
        auto  gl   = ctx.gl;
        auto  cache = ctx.cache;
        int   w = ctx.textureWidth;
        int   h = ctx.textureHeight;

        // No input — output black
        if (ctx.inputs.isEmpty() || ctx.inputs[0].textureId == 0) {
            cache->bindFboToTexture(ctx.outputTextureId);
            gl->glViewport(0, 0, w, h);
            gl->glClearColor(0, 0, 0, 1);
            gl->glClear(GL_COLOR_BUFFER_BIT);
            return;
        }

        GLuint hShader = cache->getOrCompileShader(
            "blurhq_horizontal",
            RenderResourceCache::standardVertexSource(),
            horizontalFrag());
        GLuint vShader = cache->getOrCompileShader(
            "blurhq_vertical",
            RenderResourceCache::standardVertexSource(),
            verticalFrag());

        // Intermediate texture for the horizontal pass result
        GLuint intermediate = cache->acquireTexture(w, h);

        // --- Pass 1: horizontal blur ---
        cache->bindFboToTexture(intermediate);
        ctx.useShader(hShader);
        ctx.bindTexture(hShader, "u_image", ctx.inputs[0].textureId, 0);
        gl->glUniform1f(
            gl->glGetUniformLocation(hShader, "u_radius"), data.radius);
        gl->glUniform2f(
            gl->glGetUniformLocation(hShader, "_textureSize"),
            float(w), float(h));
        ctx.drawQuad();

        // --- Pass 2: vertical blur ---
        cache->bindFboToTexture(ctx.outputTextureId);
        ctx.useShader(vShader);
        ctx.bindTexture(vShader, "u_image", intermediate, 0);
        gl->glUniform1f(
            gl->glGetUniformLocation(vShader, "u_radius"), data.radius);
        gl->glUniform2f(
            gl->glGetUniformLocation(vShader, "_textureSize"),
            float(w), float(h));
        ctx.drawQuad();

        cache->releaseTexture(intermediate);
    }

private:
    // Shared Gaussian sampling code — axis is injected per-pass
    static QString gaussianBody(const QString& axis)
    {
        return QString(R""""(
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_image;
            uniform vec2      _textureSize;
            uniform float     u_radius;

            void main()
            {
                vec2 uv   = v_texCoord;
                vec2 step = 1.0 / _textureSize;

                float sigma     = max(u_radius / 3.0, 0.001);
                float twoSigSq  = 2.0 * sigma * sigma;
                vec4  result    = vec4(0.0);
                float totalW    = 0.0;

                int radius = int(ceil(u_radius));
                for (int i = -radius; i <= radius; i++) {
                    float w     = exp(-float(i * i) / twoSigSq);
                    vec2  offset = %1 * float(i);
                    result      += texture(u_image, uv + offset * step) * w;
                    totalW      += w;
                }

                fragColor = result / max(totalW, 0.0001);
            }
        )"""").arg(axis);
    }

    static QString horizontalFrag() { return gaussianBody("vec2(1.0, 0.0)"); }
    static QString verticalFrag()   { return gaussianBody("vec2(0.0, 1.0)"); }
};

// ============================================================================
// BlurHQNode
// ============================================================================
void BlurHQNode::init()
{
    this->title = "Blur HQ";

    this->addInput("image");

    this->addFloatProp("radius", "Radius", 8.0, 0.5, 128.0, 0.5);
}

std::shared_ptr<NodeTextureRenderer> BlurHQNode::createRenderer()
{
    return std::make_shared<BlurHQRenderer>();
}

std::shared_ptr<NodeRenderData> BlurHQNode::createRenderData()
{
    auto data    = std::make_shared<BlurHQRenderData>();
    auto* prop   = dynamic_cast<FloatProp*>(getProp("radius"));
    if (prop) data->radius = static_cast<float>(prop->value);
    return data;
}
