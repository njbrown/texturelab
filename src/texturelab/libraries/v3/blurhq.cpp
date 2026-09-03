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

        GLuint intermediate = cache->acquireTexture(w, h);
        GLuint inputTex = ctx.inputs[0].textureId;

        {
            // GL_LINEAR is required for the bilinear tap trick in the shaders —
            // sampling at fractional offsets must interpolate rather than snap.
            // The guards put GL_NEAREST back on the way out: the input texture
            // belongs to the node upstream and outlives this render.
            ScopedTextureParams intermediateParams(gl, intermediate, GL_LINEAR,
                                                   GL_LINEAR);
            ScopedTextureParams inputParams(gl, inputTex, GL_LINEAR, GL_LINEAR);

            // --- Pass 1: horizontal blur ---
            cache->bindFboToTexture(intermediate);
            ctx.useShader(hShader);
            ctx.bindTexture(hShader, "u_image", inputTex, 0);
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
        }

        cache->releaseTexture(intermediate);
    }

private:
    // Shared Gaussian sampling code — axis is injected per-pass.
    //
    // Uses the bilinear tap trick: instead of sampling at each integer pixel
    // offset i, adjacent taps i and i+1 are collapsed into a single fetch at
    // the Gaussian-weighted midpoint between them.  Hardware bilinear filtering
    // then delivers the exact weighted average in one sample, halving fetch
    // count and eliminating the discrete-step banding that arises from
    // integer-only sampling on smooth gradients.
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

                // Scale radius by resolution so the blur covers the same
                // visual proportion regardless of texture size (512 = reference).
                float pixelRadius = u_radius * (_textureSize.x / 512.0);
                float sigma       = max(pixelRadius / 3.0, 0.001);
                float twoSigSq    = 2.0 * sigma * sigma;

                // Center tap: G(0) = 1, no offset needed.
                vec4  result = texture(u_image, uv);
                float totalW = 1.0;

                int iRadius = int(ceil(pixelRadius));

                // Bilinear tap trick: pair taps i and i+1, sample once at the
                // weighted midpoint.  GL_LINEAR on u_image makes the hardware
                // perform the blend.  Use float(i)*float(i) to avoid any
                // potential int overflow at large radii.
                for (int i = 1; i <= iRadius; i += 2) {
                    float fi = float(i);
                    float w0 = exp(-(fi * fi) / twoSigSq);
                    float w1 = (i + 1 <= iRadius)
                               ? exp(-((fi + 1.0) * (fi + 1.0)) / twoSigSq)
                               : 0.0;
                    float w      = w0 + w1;
                    float offset = fi + w1 / w;

                    vec2 o = %1 * offset * step;
                    result += texture(u_image, fract(uv + o)) * w;
                    result += texture(u_image, fract(uv - o)) * w;
                    totalW += 2.0 * w;
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
