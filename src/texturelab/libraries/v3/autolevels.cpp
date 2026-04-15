#include "../../graphics/noderenderer.h"
#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

#include <QOpenGLFunctions_3_2_Core>
#include <algorithm>

// GPU min/max reduction via progressive 2×2 downsampling.
// Pipeline:
//   1. Init pass    — convert full-res source to luma (per_channel=false)
//                     or keep RGB (per_channel=true)
//   2. Min chain    — halve resolution each step, computing component-wise min
//   3. Max chain    — same for max
//   Both chains converge to a 1×1 texture storing the global min/max.
//   4. Apply pass   — remap source using the 1×1 min/max values + gamma.
//
// This gives a true per-frame automatic levels without any CPU readback.
// Follow with a Curve or Map Range node for further tonal shaping.

// ============================================================================
// AutoLevelsRenderData
// ============================================================================
struct AutoLevelsRenderData : public NodeRenderData {
    float gamma      = 1.0f;
    bool  perChannel = false;
};

// ============================================================================
// AutoLevelsRenderer
// ============================================================================
class AutoLevelsRenderer : public NodeTextureRenderer {
public:
    void render(NodeRenderContext& ctx, const NodeRenderData& baseData) override
    {
        auto& data  = static_cast<const AutoLevelsRenderData&>(baseData);
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

        GLuint srcTex    = ctx.inputs[0].textureId;
        GLuint initSh    = cache->getOrCompileShader("al_init",  stdVert(), initFrag());
        GLuint minSh     = cache->getOrCompileShader("al_min",   stdVert(), reduceFrag(true));
        GLuint maxSh     = cache->getOrCompileShader("al_max",   stdVert(), reduceFrag(false));
        GLuint applySh   = cache->getOrCompileShader("al_apply", stdVert(), applyFrag());

        // --- 1. Init pass (full resolution) ---
        // per_channel=false → store luma in RGB so all 3 channels reduce identically
        // per_channel=true  → keep RGB as-is
        GLuint initTex = cache->acquireTexture(w, h);
        cache->bindFboToTexture(initTex);
        ctx.useShader(initSh);
        ctx.bindTexture(initSh, "u_src", srcTex, 0);
        gl->glUniform1i(
            gl->glGetUniformLocation(initSh, "u_per_channel"),
            data.perChannel ? 1 : 0);
        ctx.drawQuad();

        // --- 2 & 3. Min/Max reduction chains ---
        GLuint minTex = reduce(ctx, cache, gl, initTex, w, h, minSh);
        GLuint maxTex = reduce(ctx, cache, gl, initTex, w, h, maxSh);

        cache->releaseTexture(initTex);

        // --- 4. Apply pass (full resolution) ---
        cache->bindFboToTexture(ctx.outputTextureId);
        ctx.useShader(applySh);
        ctx.bindTexture(applySh, "u_src",        srcTex,  0);
        ctx.bindTexture(applySh, "u_min",        minTex,  1);
        ctx.bindTexture(applySh, "u_max",        maxTex,  2);
        gl->glUniform1f(
            gl->glGetUniformLocation(applySh, "u_gamma"),
            data.gamma);
        gl->glUniform1i(
            gl->glGetUniformLocation(applySh, "u_per_channel"),
            data.perChannel ? 1 : 0);
        ctx.drawQuad();

        cache->releaseTexture(minTex);
        cache->releaseTexture(maxTex);
    }

private:
    // Progressively halve the texture, applying the given reduce shader
    // (min or max) at each step.  Returns a 1×1 texture the caller must release.
    static GLuint reduce(NodeRenderContext& ctx, RenderResourceCache* cache,
                         QOpenGLFunctions_3_2_Core* gl,
                         GLuint srcTex, int w, int h, GLuint shader)
    {
        GLuint current      = srcTex;
        int    cw = w, ch = h;
        bool   ownsCurrent  = false;

        while (cw > 1 || ch > 1) {
            int    nextW = std::max(cw / 2, 1);
            int    nextH = std::max(ch / 2, 1);
            GLuint next  = cache->acquireTexture(nextW, nextH);

            // Bind FBO manually — we don't use ctx.useShader here because
            // it would force the viewport to the node's final output size.
            cache->bindFboToTexture(next);
            gl->glViewport(0, 0, nextW, nextH);
            gl->glUseProgram(shader);

            // Tell the shader how big the SOURCE texture is so it can
            // compute the correct half-texel offset for 2×2 sampling.
            gl->glUniform2f(
                gl->glGetUniformLocation(shader, "u_src_size"),
                float(cw), float(ch));

            ctx.bindTexture(shader, "u_src", current, 0);
            ctx.drawQuad();

            if (ownsCurrent) cache->releaseTexture(current);
            current     = next;
            ownsCurrent = true;
            cw = nextW;
            ch = nextH;
        }

        return current; // 1×1 result, caller must release
    }

    static QString stdVert() { return RenderResourceCache::standardVertexSource(); }

    // 1. Init: convert source to values suitable for reduction
    static QString initFrag()
    {
        return R""""(
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_src;
            uniform int       u_per_channel;

            void main()
            {
                vec4 c = texture(u_src, v_texCoord);
                if (u_per_channel == 1) {
                    // Keep RGB — reduce each channel independently
                    fragColor = vec4(c.rgb, 1.0);
                } else {
                    // Replicate luma to RGB so all channels carry the same value;
                    // the min/max of any channel then equals the global luma min/max.
                    float luma = dot(c.rgb, vec3(0.2126, 0.7152, 0.0722));
                    fragColor  = vec4(luma, luma, luma, 1.0);
                }
            }
        )"""";
    }

    // 2/3. Reduction: component-wise min or max over a 2×2 block of the source
    static QString reduceFrag(bool isMin)
    {
        QString op = isMin ? "min" : "max";
        return QString(R""""(
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_src;
            uniform vec2      u_src_size; // size of the INPUT texture for this pass

            void main()
            {
                // Half-texel offset in source-texture space centres the 4 samples
                // on the 2×2 block that maps to this output pixel.
                vec2 ht = 0.5 / u_src_size;
                vec2 uv = v_texCoord;

                vec4 s00 = texture(u_src, uv + vec2(-ht.x, -ht.y));
                vec4 s10 = texture(u_src, uv + vec2( ht.x, -ht.y));
                vec4 s01 = texture(u_src, uv + vec2(-ht.x,  ht.y));
                vec4 s11 = texture(u_src, uv + vec2( ht.x,  ht.y));

                fragColor = %1(%1(s00, s10), %1(s01, s11));
            }
        )"""").arg(op);
    }

    // 4. Apply: remap using the 1×1 min/max textures + gamma
    static QString applyFrag()
    {
        return R""""(
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_src;
            uniform sampler2D u_min;
            uniform sampler2D u_max;
            uniform float     u_gamma;
            uniform int       u_per_channel;

            float remap(float v, float lo, float hi) {
                float range = max(hi - lo, 0.001);
                float t = clamp((v - lo) / range, 0.0, 1.0);
                // Gamma > 1 brightens midtones; gamma < 1 darkens.
                return (u_gamma > 0.999 && u_gamma < 1.001)
                    ? t
                    : pow(t, 1.0 / max(u_gamma, 0.001));
            }

            void main()
            {
                vec4  src = texture(u_src, v_texCoord);
                // 1×1 textures — sample at the centre; exact UV doesn't matter.
                vec4  lo  = texture(u_min, vec2(0.5));
                vec4  hi  = texture(u_max, vec2(0.5));

                vec3 result;
                if (u_per_channel == 1) {
                    // Independent per-channel remap (can shift colour balance)
                    result.r = remap(src.r, lo.r, hi.r);
                    result.g = remap(src.g, lo.g, hi.g);
                    result.b = remap(src.b, lo.b, hi.b);
                } else {
                    // Luminance-preserving remap: scale RGB uniformly so hue is kept
                    float luma    = dot(src.rgb, vec3(0.2126, 0.7152, 0.0722));
                    float newLuma = remap(luma, lo.r, hi.r);
                    float scale   = (luma > 0.0001) ? newLuma / luma : 0.0;
                    result        = clamp(src.rgb * scale, 0.0, 1.0);
                }

                fragColor = vec4(result, src.a);
            }
        )"""";
    }
};

// ============================================================================
// AutoLevelsNode
// ============================================================================
void AutoLevelsNode::init()
{
    this->title = "Auto Levels";

    this->addInput("image");

    this->addFloatProp("gamma",       "Midpoint Gamma",  1.0,  0.1, 4.0,  0.05);
    this->addBoolProp ("per_channel", "Per Channel",     false);
}

std::shared_ptr<NodeTextureRenderer> AutoLevelsNode::createRenderer()
{
    return std::make_shared<AutoLevelsRenderer>();
}

std::shared_ptr<NodeRenderData> AutoLevelsNode::createRenderData()
{
    auto data = std::make_shared<AutoLevelsRenderData>();
    if (auto* p = dynamic_cast<FloatProp*>(getProp("gamma")))
        data->gamma = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<BoolProp*>(getProp("per_channel")))
        data->perChannel = p->value;
    return data;
}
