#include "../../graphics/noderenderer.h"
#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

#include <QOpenGLFunctions_3_2_Core>
#include <algorithm>
#include <cmath>

// Vogel, "A better way to construct the sunflower head," Math. Biosciences 1979
//   (golden-angle disc — the sample distribution used here)
// Jimenez et al., "Next Generation Post Processing in Call of Duty: Advanced
//   Warfare," SIGGRAPH 2014 (interleaved gradient noise + matched 4x4 filter)
// Crassin et al., "Interactive Indirect Illumination Using Voxel Cone Tracing,"
//   PG 2011 (mip level chosen from cone footprint)
//
// Fast AO — the high-quality counterpart to the single-pass Ambient
// Occlusion node.  Four ideas the single-pass version cannot express:
//
//   1. Cone taps.  A height pyramid is built once, and each tap reads the mip
//      level matching its footprint.  A far tap then represents the average
//      of the region it stands for instead of one arbitrary texel, which is
//      where most of the remaining noise and all of the large-radius aliasing
//      came from.
//   2. Progressive accumulation.  This is an offline texture tool, not a
//      16 ms frame budget, so quality is bought with passes rather than with
//      a longer inner loop: N additive passes, each laying down a differently
//      rotated spiral, converge to a noise-free result while Draft stays
//      interactive.
//   3. Two scales.  A broad occlusion term and a tight cavity term are
//      accumulated into separate channels and combined at resolve, because a
//      single radius always compromises between contact darkening and broad
//      ambient shading.
//   4. Bilateral resolve.  The per-pixel spiral rotation comes from
//      interleaved gradient noise, whose 4x4 tile the resolve pass's 4x4 box
//      cancels almost exactly — worth roughly 16x the sample count for the
//      cost of one pass.  The box is height-guided so it never blurs
//      occlusion across a step in the surface.

// ============================================================================
// FastAORenderData
// ============================================================================
struct FastAORenderData : public NodeRenderData {
    float radius = 0.08f;
    int samples = 48;
    int quality = 1; // enum index -> pass count
    float intensity = 1.0f;
    float bias = 0.02f;
    float heightScale = 1.0f;
    float detail = 0.35f;
    float detailScale = 0.15f;
    float denoise = 0.5f;
    float contrast = 1.0f;
    bool cone = true;
};

// ============================================================================
// FastAORenderer
// ============================================================================
class FastAORenderer : public NodeTextureRenderer {
public:
    void render(NodeRenderContext& ctx, const NodeRenderData& baseData) override
    {
        auto& data = static_cast<const FastAORenderData&>(baseData);
        auto gl = ctx.gl;
        auto cache = ctx.cache;
        int w = ctx.textureWidth;
        int h = ctx.textureHeight;

        // No input — unoccluded is white, not black.
        if (ctx.inputs.isEmpty() || ctx.inputs[0].textureId == 0) {
            cache->bindFboToTexture(ctx.outputTextureId);
            gl->glViewport(0, 0, w, h);
            gl->glClearColor(1, 1, 1, 1);
            gl->glClear(GL_COLOR_BUFFER_BIT);
            return;
        }

        GLuint prepShader = cache->getOrCompileShader(
            "fastao_prep", RenderResourceCache::standardVertexSource(),
            prepFrag());
        GLuint aoShader = cache->getOrCompileShader(
            "fastao_ao", RenderResourceCache::standardVertexSource(),
            aoFrag());
        GLuint resolveShader = cache->getOrCompileShader(
            "fastao_resolve", RenderResourceCache::standardVertexSource(),
            resolveFrag());

        GLuint heightTex = cache->acquireTexture(w, h);
        GLuint accumTex = cache->acquireTexture(w, h);

        // --- Pass 1: linearise the height into its own texture -------------
        // Applying height_scale once here keeps it out of the inner loop, and
        // gives us a texture we own and may re-parameterise freely.
        cache->bindFboToTexture(heightTex);
        ctx.useShader(prepShader);
        ctx.bindTexture(prepShader, "u_input", ctx.inputs[0].textureId, 0);
        gl->glUniform1f(gl->glGetUniformLocation(prepShader, "u_heightScale"),
                        data.heightScale);
        ctx.drawQuad();

        // --- Pass 2: accumulate N rotated AO passes ------------------------
        const int passes = passCountFor(data.quality);

        // Point the FBO at the accumulation target *before* generating the
        // pyramid: generating mips for a texture still attached to the bound
        // framebuffer is a feedback loop the spec leaves undefined.
        cache->bindFboToTexture(accumTex);
        gl->glViewport(0, 0, w, h);
        gl->glClearColor(0, 0, 0, 0);
        gl->glClear(GL_COLOR_BUFFER_BIT);

        {
            // Trilinear + REPEAT for the duration of the AO passes: the cone
            // taps read down the pyramid, and taps have to wrap because the AO
            // of a tiling texture has to tile too. The guard restores the
            // pool's defaults on the way out. The mip storage stays allocated
            // on the texture, but with GL_NEAREST it is never sampled again.
            ScopedTextureParams heightParams(gl, heightTex,
                                             GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
                                             GL_REPEAT);
            // The guard leaves nothing bound, so bind explicitly here —
            // glGenerateMipmap acts on whatever is bound to the target.
            gl->glBindTexture(GL_TEXTURE_2D, heightTex);
            gl->glGenerateMipmap(GL_TEXTURE_2D);
            gl->glBindTexture(GL_TEXTURE_2D, 0);

            // Additive blending is what makes accumulation a single texture
            // instead of a ping-pong pair; the resolve divides by the pass
            // count.
            gl->glEnable(GL_BLEND);
            gl->glBlendFunc(GL_ONE, GL_ONE);

            for (int p = 0; p < passes; p++) {
                ctx.useShader(aoShader);
                ctx.bindTexture(aoShader, "u_height", heightTex, 0);
                gl->glUniform1f(gl->glGetUniformLocation(aoShader, "u_radius"),
                                data.radius);
                gl->glUniform1i(gl->glGetUniformLocation(aoShader, "u_samples"),
                                data.samples);
                gl->glUniform1f(gl->glGetUniformLocation(aoShader, "u_bias"),
                                data.bias);
                gl->glUniform1f(
                    gl->glGetUniformLocation(aoShader, "u_detailScale"),
                    data.detailScale);
                gl->glUniform1i(gl->glGetUniformLocation(aoShader, "u_detail"),
                                data.detail > 0.0f ? 1 : 0);
                gl->glUniform1i(gl->glGetUniformLocation(aoShader, "u_cone"),
                                data.cone ? 1 : 0);
                gl->glUniform1i(gl->glGetUniformLocation(aoShader, "u_pass"), p);
                ctx.drawQuad();
            }

            gl->glDisable(GL_BLEND);

            // --- Pass 3: bilateral resolve ---------------------------------
            cache->bindFboToTexture(ctx.outputTextureId);
            ctx.useShader(resolveShader);
            ctx.bindTexture(resolveShader, "u_accum", accumTex, 0);
            ctx.bindTexture(resolveShader, "u_height", heightTex, 1);
            gl->glUniform1f(
                gl->glGetUniformLocation(resolveShader, "u_invPasses"),
                1.0f / float(passes));
            gl->glUniform1f(
                gl->glGetUniformLocation(resolveShader, "u_intensity"),
                data.intensity);
            gl->glUniform1f(gl->glGetUniformLocation(resolveShader, "u_detail"),
                            data.detail);
            gl->glUniform1f(gl->glGetUniformLocation(resolveShader, "u_denoise"),
                            data.denoise);
            gl->glUniform1f(
                gl->glGetUniformLocation(resolveShader, "u_contrast"),
                data.contrast);
            ctx.drawQuad();
        }

        cache->releaseTexture(heightTex);
        cache->releaseTexture(accumTex);
    }

private:
    static int passCountFor(int qualityIndex)
    {
        switch (qualityIndex) {
        case 0:
            return 1; // Draft
        case 1:
            return 2; // Medium
        case 2:
            return 4; // High
        case 3:
            return 8; // Ultra
        default:
            return 2;
        }
    }

    // Copy the height channel out, pre-scaled, so the AO pass reads a texture
    // we control the filtering and wrap mode of.
    static QString prepFrag()
    {
        return R""""(
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_input;
            uniform float     u_heightScale;

            void main()
            {
                float h = texture(u_input, v_texCoord).r * u_heightScale;
                fragColor = vec4(h, h, h, 1.0);
            }
        )"""";
    }

    // One accumulation pass: a rotated Vogel disc, sampled as cones, writing
    // the broad term to R and the cavity term to G.
    static QString aoFrag()
    {
        return R""""(
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_height;
            uniform vec2      _textureSize;
            uniform float     u_radius;
            uniform int       u_samples;
            uniform float     u_bias;
            uniform float     u_detailScale;
            uniform int       u_detail;
            uniform int       u_cone;
            uniform int       u_pass;

            #define TAU          6.28318530718
            #define GOLDEN_ANGLE 2.39996322973

            // Interleaved gradient noise: over any 4x4 pixel tile this yields
            // 16 evenly spread values, which is exactly what the resolve
            // pass's 4x4 box is built to average back out.
            float ign(vec2 p)
            {
                return fract(52.9829189 *
                             fract(dot(p, vec2(0.06711056, 0.00583715))));
            }

            void main()
            {
                float sampleCount = float(u_samples);

                // Radius is in UV; keep the disc circular in texel space.
                vec2 aspect = vec2(_textureSize.y / _textureSize.x, 1.0);

                // Golden-ratio offset per pass, so N passes lay down N
                // maximally separated rotations of the same spiral rather
                // than N arbitrary ones.
                float rot = ign(gl_FragCoord.xy +
                                vec2(float(u_pass) * 5.588238)) * TAU;

                float centerH = textureLod(u_height, v_texCoord, 0.0).r;

                // N taps over a disc of radius R cannot resolve detail finer
                // than their spacing, so prefilter each tap to roughly its
                // own footprint instead of point-sampling one texel out of
                // the region it is meant to stand for.
                float coneScale = (u_cone == 1) ? 2.0 / sqrt(sampleCount) : 0.0;
                float texelScale = max(_textureSize.x, _textureSize.y);

                float occBroad  = 0.0;
                float occFine   = 0.0;
                float weightSum = 0.0;

                for (int i = 0; i < u_samples; i++)
                {
                    float t     = (float(i) + 0.5) / sampleCount;
                    float angle = float(i) * GOLDEN_ANGLE + rot;
                    vec2  dir   = vec2(cos(angle), sin(angle)) * aspect;

                    // Smooth window over the disc edge, so a feature crossing
                    // the radius boundary fades in instead of popping.
                    float w = 1.0 - t;

                    // --- broad term ---
                    // sqrt(t) distributes the taps uniformly over the disc
                    // area rather than over the radius.
                    float dist = sqrt(t) * u_radius;
                    float lod  = log2(max(1.0, dist * texelScale * coneScale));
                    float dh   = textureLod(u_height, v_texCoord + dir * dist,
                                            lod).r - centerH;
                    // sin(atan(dh / dist)) — the elevation angle of this
                    // neighbour. Bounded in [0,1] and scale-aware, unlike a
                    // raw height difference.
                    occBroad += clamp(dh * inversesqrt(dh * dh + dist * dist)
                                      - u_bias, 0.0, 1.0) * w;

                    // --- cavity term: the same spiral, tightened ---
                    if (u_detail == 1) {
                        float distF = dist * u_detailScale;
                        float lodF  = log2(max(1.0, distF * texelScale * coneScale));
                        float dhF   = textureLod(u_height,
                                                 v_texCoord + dir * distF,
                                                 lodF).r - centerH;
                        occFine += clamp(dhF * inversesqrt(dhF * dhF + distF * distF)
                                         - u_bias, 0.0, 1.0) * w;
                    }

                    weightSum += w;
                }

                float inv = 1.0 / max(weightSum, 0.0001);
                fragColor = vec4(occBroad * inv, occFine * inv, 0.0, 0.0);
            }
        )"""";
    }

    // Average the accumulation, denoise it, combine the two scales, invert.
    static QString resolveFrag()
    {
        return R""""(
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_accum;
            uniform sampler2D u_height;
            uniform vec2      _textureSize;
            uniform float     u_invPasses;
            uniform float     u_intensity;
            uniform float     u_detail;
            uniform float     u_denoise;
            uniform float     u_contrast;

            void main()
            {
                vec2  texel   = 1.0 / _textureSize;
                float centerH = textureLod(u_height, v_texCoord, 0.0).r;

                vec2 raw = texture(u_accum, v_texCoord).rg * u_invPasses;

                // 4x4 box aligned to the interleaved-gradient-noise tile, so
                // the 16 complementary rotations laid down by the AO pass
                // cancel. Height-guided: occlusion is never blurred across a
                // step in the surface, which is what a plain blur would eat.
                vec2  sum  = vec2(0.0);
                float wsum = 0.0;
                for (int y = -1; y <= 2; y++) {
                    for (int x = -1; x <= 2; x++) {
                        // fract() wraps the tap — pooled textures are
                        // CLAMP_TO_EDGE and the result has to tile.
                        vec2 uv = fract(v_texCoord +
                                        vec2(float(x), float(y)) * texel);
                        float hs = textureLod(u_height, uv, 0.0).r;
                        float w  = exp(-abs(hs - centerH) * 32.0);
                        sum  += texture(u_accum, uv).rg * u_invPasses * w;
                        wsum += w;
                    }
                }

                vec2 ao = mix(raw, sum / max(wsum, 0.0001), u_denoise);

                float broad = 1.0 - clamp(ao.r * u_intensity, 0.0, 1.0);
                float fine  = 1.0 - clamp(ao.g * u_intensity, 0.0, 1.0);

                // The cavity term reads as a separate layer of shading — it
                // multiplies the broad occlusion rather than adding to it.
                float result = broad * mix(1.0, fine, u_detail);
                result = pow(max(result, 0.0), u_contrast);

                fragColor = vec4(vec3(result), 1.0);
            }
        )"""";
    }
};

// ============================================================================
// FastAONode
// ============================================================================
void FastAONode::init()
{
    this->title = "Fast AO";

    this->addInput("height");

    this->addFloatProp("radius", "Radius", 0.01, 0.001, 1.0, 0.005);
    this->addIntProp("samples", "Samples", 48, 8, 256, 8);
    auto qualityEnum =
        this->addEnumProp("quality", "Quality",
                          QList<QString>{"Draft", "Medium", "High", "Ultra"});
    qualityEnum->index = 1; // Medium
    this->addFloatProp("intensity", "Intensity", 1.0, 0.0, 5.0, 0.1);
    this->addFloatProp("bias", "Bias", 0.02, 0.0, 0.5, 0.005);
    this->addFloatProp("height_scale", "Height Scale", 1.0, 0.01, 8.0, 0.01);
    this->addFloatProp("detail", "Detail", 0.35, 0.0, 1.0, 0.01);
    this->addFloatProp("detail_scale", "Detail Scale", 0.15, 0.02, 0.5, 0.01);
    this->addFloatProp("denoise", "Denoise", 0.5, 0.0, 1.0, 0.01);
    this->addFloatProp("contrast", "Contrast", 0.5, 0.25, 4.0, 0.05);
    this->addBoolProp("cone", "Cone Filtering", true);
}

std::shared_ptr<NodeTextureRenderer> FastAONode::createRenderer()
{
    return std::make_shared<FastAORenderer>();
}

std::shared_ptr<NodeRenderData> FastAONode::createRenderData()
{
    auto data = std::make_shared<FastAORenderData>();

    if (auto* p = dynamic_cast<FloatProp*>(getProp("radius")))
        data->radius = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<IntProp*>(getProp("samples")))
        data->samples = p->value;
    if (auto* p = dynamic_cast<EnumProp*>(getProp("quality")))
        data->quality = p->index;
    if (auto* p = dynamic_cast<FloatProp*>(getProp("intensity")))
        data->intensity = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<FloatProp*>(getProp("bias")))
        data->bias = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<FloatProp*>(getProp("height_scale")))
        data->heightScale = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<FloatProp*>(getProp("detail")))
        data->detail = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<FloatProp*>(getProp("detail_scale")))
        data->detailScale = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<FloatProp*>(getProp("denoise")))
        data->denoise = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<FloatProp*>(getProp("contrast")))
        data->contrast = static_cast<float>(p->value);
    if (auto* p = dynamic_cast<BoolProp*>(getProp("cone")))
        data->cone = p->value;

    return data;
}
