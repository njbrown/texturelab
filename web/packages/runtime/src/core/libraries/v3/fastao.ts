// Ported from src/texturelab/libraries/v3/fastao.cpp.
// Keep the GLSL and props in sync with the C++ when either changes.

// Vogel, "A better way to construct the sunflower head," Math. Biosciences 1979
// Jimenez et al., "Next Generation Post Processing in Call of Duty: Advanced
//   Warfare," SIGGRAPH 2014 (interleaved gradient noise + matched 4x4 filter)
// Crassin et al., "Interactive Indirect Illumination Using Voxel Cone Tracing,"
//   PG 2011 (mip level chosen from cone footprint)
//
// Fast AO — the high-quality counterpart to the single-pass Ambient Occlusion
// node: cone taps into a height pyramid, progressive accumulation over N
// rotated passes, separate broad and cavity terms, and a bilateral resolve.

import {
    standardVertexSource,
    withTextureParams,
    type NodeRenderContext,
    type NodeTextureRenderer,
} from "../../gl/noderenderer.js";
import { TextureNode } from "../../node.js";
import type { BoolProp, EnumProp, FloatProp, IntProp } from "../../props.js";

interface FastAORenderData {
    radius: number;
    samples: number;
    quality: number; // enum index -> pass count
    intensity: number;
    bias: number;
    heightScale: number;
    detail: number;
    detailScale: number;
    denoise: number;
    contrast: number;
    cone: boolean;
}

class FastAORenderer implements NodeTextureRenderer<FastAORenderData> {
    render(ctx: NodeRenderContext, data: FastAORenderData): void {
        const gl = ctx.gl;
        const cache = ctx.cache;
        const w = ctx.textureWidth;
        const h = ctx.textureHeight;

        // No input — unoccluded is white, not black.
        if (ctx.inputs.length === 0) {
            ctx.clearOutput(1, 1, 1, 1);
            return;
        }

        const prepShader = cache.getOrCompileShader("fastao_prep", standardVertexSource, prepFrag);
        const aoShader = cache.getOrCompileShader("fastao_ao", standardVertexSource, aoFrag);
        const resolveShader = cache.getOrCompileShader("fastao_resolve", standardVertexSource, resolveFrag);

        const heightTex = cache.acquireTexture(w, h);
        const accumTex = cache.acquireTexture(w, h);

        // --- Pass 1: linearise the height into its own texture ---
        cache.bindFboToTexture(heightTex);
        ctx.useShader(prepShader);
        ctx.bindTexture(prepShader, "u_input", ctx.inputs[0]!.texture, 0);
        gl.uniform1f(gl.getUniformLocation(prepShader, "u_heightScale"), data.heightScale);
        ctx.drawQuad();

        // --- Pass 2: accumulate N rotated AO passes ---
        const passes = passCountFor(data.quality);

        // Point the FBO at the accumulation target *before* generating the
        // pyramid: generating mips for a texture still attached to the bound
        // framebuffer is a feedback loop.
        cache.bindFboToTexture(accumTex);
        gl.viewport(0, 0, w, h);
        gl.clearColor(0, 0, 0, 0);
        gl.clear(gl.COLOR_BUFFER_BIT);

        // Trilinear + REPEAT for the duration of the AO passes: the cone taps
        // read down the pyramid, and taps have to wrap because the AO of a
        // tiling texture has to tile too.
        withTextureParams(
            gl,
            [{ texture: heightTex, minFilter: gl.LINEAR_MIPMAP_LINEAR, magFilter: gl.LINEAR, wrap: gl.REPEAT }],
            () => {
                gl.bindTexture(gl.TEXTURE_2D, heightTex);
                gl.generateMipmap(gl.TEXTURE_2D);
                gl.bindTexture(gl.TEXTURE_2D, null);

                // Additive blending makes accumulation a single texture
                // instead of a ping-pong pair; the resolve divides by the
                // pass count.
                gl.enable(gl.BLEND);
                gl.blendFunc(gl.ONE, gl.ONE);

                for (let p = 0; p < passes; p++) {
                    ctx.useShader(aoShader);
                    ctx.bindTexture(aoShader, "u_height", heightTex, 0);
                    gl.uniform1f(gl.getUniformLocation(aoShader, "u_radius"), data.radius);
                    gl.uniform1i(gl.getUniformLocation(aoShader, "u_samples"), data.samples);
                    gl.uniform1f(gl.getUniformLocation(aoShader, "u_bias"), data.bias);
                    gl.uniform1f(gl.getUniformLocation(aoShader, "u_detailScale"), data.detailScale);
                    gl.uniform1i(gl.getUniformLocation(aoShader, "u_detail"), data.detail > 0 ? 1 : 0);
                    gl.uniform1i(gl.getUniformLocation(aoShader, "u_cone"), data.cone ? 1 : 0);
                    gl.uniform1i(gl.getUniformLocation(aoShader, "u_pass"), p);
                    ctx.drawQuad();
                }

                gl.disable(gl.BLEND);

                // --- Pass 3: bilateral resolve ---
                cache.bindFboToTexture(ctx.outputTexture);
                ctx.useShader(resolveShader);
                ctx.bindTexture(resolveShader, "u_accum", accumTex, 0);
                ctx.bindTexture(resolveShader, "u_height", heightTex, 1);
                gl.uniform1f(gl.getUniformLocation(resolveShader, "u_invPasses"), Math.fround(1 / passes));
                gl.uniform1f(gl.getUniformLocation(resolveShader, "u_intensity"), data.intensity);
                gl.uniform1f(gl.getUniformLocation(resolveShader, "u_detail"), data.detail);
                gl.uniform1f(gl.getUniformLocation(resolveShader, "u_denoise"), data.denoise);
                gl.uniform1f(gl.getUniformLocation(resolveShader, "u_contrast"), data.contrast);
                ctx.drawQuad();
            },
        );

        cache.releaseTexture(heightTex);
        cache.releaseTexture(accumTex);
    }
}

function passCountFor(qualityIndex: number): number {
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

// Copy the height channel out, pre-scaled, so the AO pass reads a texture we
// control the filtering and wrap mode of.
const prepFrag = /* glsl */ `
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
        `;

// One accumulation pass: a rotated Vogel disc, sampled as cones, writing the
// broad term to R and the cavity term to G.
const aoFrag = /* glsl */ `
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
        `;

// Average the accumulation, denoise it, combine the two scales, invert.
const resolveFrag = /* glsl */ `
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
        `;

export class FastAONode extends TextureNode {
    override init(): void {
        this.title = "Fast AO";
        this.addInput("height");
        this.addFloatProp("radius", "Radius", 0.01, 0.001, 1.0, 0.005);
        this.addIntProp("samples", "Samples", 48, 8, 256, 8);
        const qualityEnum = this.addEnumProp("quality", "Quality", ["Draft", "Medium", "High", "Ultra"]);
        qualityEnum.index = 1; // Medium
        this.addFloatProp("intensity", "Intensity", 1.0, 0.0, 5.0, 0.1);
        this.addFloatProp("bias", "Bias", 0.02, 0.0, 0.5, 0.005);
        this.addFloatProp("height_scale", "Height Scale", 1.0, 0.01, 8.0, 0.01);
        this.addFloatProp("detail", "Detail", 0.35, 0.0, 1.0, 0.01);
        this.addFloatProp("detail_scale", "Detail Scale", 0.15, 0.02, 0.5, 0.01);
        this.addFloatProp("denoise", "Denoise", 0.5, 0.0, 1.0, 0.01);
        this.addFloatProp("contrast", "Contrast", 0.5, 0.25, 4.0, 0.05);
        this.addBoolProp("cone", "Cone Filtering", true);
    }

    override createRenderer(): NodeTextureRenderer<FastAORenderData> {
        return new FastAORenderer();
    }

    override createRenderData(): FastAORenderData {
        const f = (name: string, fallback: number) => {
            const p = this.getProp(name) as FloatProp | undefined;
            return p ? Math.fround(p.value) : fallback;
        };
        const samples = this.getProp("samples") as IntProp | undefined;
        const quality = this.getProp("quality") as EnumProp | undefined;
        const cone = this.getProp("cone") as BoolProp | undefined;
        return {
            radius: f("radius", 0.08),
            samples: samples ? samples.value : 48,
            quality: quality ? quality.index : 1,
            intensity: f("intensity", 1.0),
            bias: f("bias", 0.02),
            heightScale: f("height_scale", 1.0),
            detail: f("detail", 0.35),
            detailScale: f("detail_scale", 0.15),
            denoise: f("denoise", 0.5),
            contrast: f("contrast", 1.0),
            cone: cone ? cone.value : true,
        };
    }
}
