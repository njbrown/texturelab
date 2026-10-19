// Ported from src/texturelab/libraries/v3/autolevels.cpp.
// Keep the GLSL and props in sync with the C++ when either changes.

// GPU min/max reduction via progressive 2×2 downsampling.
// Pipeline:
//   1. Init pass    — convert full-res source to luma (per_channel=false)
//                     or keep RGB (per_channel=true)
//   2. Min chain    — halve resolution each step, computing component-wise min
//   3. Max chain    — same for max
//   Both chains converge to a 1×1 texture storing the global min/max.
//   4. Apply pass   — remap source using the 1×1 min/max values + gamma.

import {
    standardVertexSource,
    type NodeRenderContext,
    type NodeTextureRenderer,
    type RenderResourceCache,
} from "../../gl/noderenderer.js";
import { TextureNode } from "../../node.js";
import type { BoolProp, FloatProp } from "../../props.js";

interface AutoLevelsRenderData {
    gamma: number;
    perChannel: boolean;
}

class AutoLevelsRenderer implements NodeTextureRenderer<AutoLevelsRenderData> {
    render(ctx: NodeRenderContext, data: AutoLevelsRenderData): void {
        const gl = ctx.gl;
        const cache = ctx.cache;
        const w = ctx.textureWidth;
        const h = ctx.textureHeight;

        if (ctx.inputs.length === 0) {
            ctx.clearOutput(0, 0, 0, 1);
            return;
        }

        const srcTex = ctx.inputs[0]!.texture;
        const initSh = cache.getOrCompileShader("al_init", standardVertexSource, initFrag());
        const minSh = cache.getOrCompileShader("al_min", standardVertexSource, reduceFrag(true));
        const maxSh = cache.getOrCompileShader("al_max", standardVertexSource, reduceFrag(false));
        const applySh = cache.getOrCompileShader("al_apply", standardVertexSource, applyFrag());

        // --- 1. Init pass (full resolution) ---
        const initTex = cache.acquireTexture(w, h);
        cache.bindFboToTexture(initTex);
        ctx.useShader(initSh);
        ctx.bindTexture(initSh, "u_src", srcTex, 0);
        gl.uniform1i(gl.getUniformLocation(initSh, "u_per_channel"), data.perChannel ? 1 : 0);
        ctx.drawQuad();

        // --- 2 & 3. Min/Max reduction chains ---
        const minTex = reduce(ctx, cache, gl, initTex, w, h, minSh);
        const maxTex = reduce(ctx, cache, gl, initTex, w, h, maxSh);

        cache.releaseTexture(initTex);

        // --- 4. Apply pass (full resolution) ---
        cache.bindFboToTexture(ctx.outputTexture);
        ctx.useShader(applySh);
        ctx.bindTexture(applySh, "u_src", srcTex, 0);
        ctx.bindTexture(applySh, "u_min", minTex, 1);
        ctx.bindTexture(applySh, "u_max", maxTex, 2);
        gl.uniform1f(gl.getUniformLocation(applySh, "u_gamma"), data.gamma);
        gl.uniform1i(gl.getUniformLocation(applySh, "u_per_channel"), data.perChannel ? 1 : 0);
        ctx.drawQuad();

        cache.releaseTexture(minTex);
        cache.releaseTexture(maxTex);
    }
}

// Progressively halve the texture, applying the given reduce shader (min or
// max) at each step. Returns a 1×1 texture the caller must release.
function reduce(
    ctx: NodeRenderContext,
    cache: RenderResourceCache,
    gl: WebGL2RenderingContext,
    srcTex: WebGLTexture,
    w: number,
    h: number,
    shader: WebGLProgram,
): WebGLTexture {
    let current = srcTex;
    let cw = w;
    let ch = h;
    let ownsCurrent = false;

    while (cw > 1 || ch > 1) {
        const nextW = Math.max(Math.trunc(cw / 2), 1);
        const nextH = Math.max(Math.trunc(ch / 2), 1);
        const next = cache.acquireTexture(nextW, nextH);

        // Bind FBO manually — ctx.useShader would force the viewport to the
        // node's final output size.
        cache.bindFboToTexture(next);
        gl.viewport(0, 0, nextW, nextH);
        gl.useProgram(shader);

        // Tell the shader how big the SOURCE texture is so it can compute the
        // correct half-texel offset for 2×2 sampling.
        gl.uniform2f(gl.getUniformLocation(shader, "u_src_size"), cw, ch);

        ctx.bindTexture(shader, "u_src", current, 0);
        ctx.drawQuad();

        if (ownsCurrent) cache.releaseTexture(current);
        current = next;
        ownsCurrent = true;
        cw = nextW;
        ch = nextH;
    }

    return current; // 1×1 result, caller must release
}

// 1. Init: convert source to values suitable for reduction
const initFrag = () => /* glsl */ `
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
        `;

// 2/3. Reduction: component-wise min or max over a 2×2 block of the source
function reduceFrag(isMin: boolean): string {
    const op = isMin ? "min" : "max";
    return /* glsl */ `
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

                fragColor = ${op}(${op}(s00, s10), ${op}(s01, s11));
            }
        `;
}

// 4. Apply: remap using the 1×1 min/max textures + gamma
const applyFrag = () => /* glsl */ `
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
        `;

export class AutoLevelsNode extends TextureNode {
    override init(): void {
        this.title = "Auto Levels";
        this.addInput("image");
        this.addFloatProp("gamma", "Midpoint Gamma", 1.0, 0.1, 4.0, 0.05);
        this.addBoolProp("per_channel", "Per Channel", false);
    }

    override createRenderer(): NodeTextureRenderer<AutoLevelsRenderData> {
        return new AutoLevelsRenderer();
    }

    override createRenderData(): AutoLevelsRenderData {
        const gamma = this.getProp("gamma") as FloatProp | undefined;
        const perChannel = this.getProp("per_channel") as BoolProp | undefined;
        return {
            gamma: gamma ? Math.fround(gamma.value) : 1.0,
            perChannel: perChannel ? perChannel.value : false,
        };
    }
}
