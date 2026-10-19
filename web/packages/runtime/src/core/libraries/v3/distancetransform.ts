// Ported from src/texturelab/libraries/v3/distancetransform.cpp.
// Keep the GLSL and props in sync with the C++ when either changes.

// Rong & Tan, "Jump Flooding in GPU with Applications to Voronoi Diagram and
//   Distance Transform," I3D 2006
//
// Jump Flooding Algorithm (JFA) Euclidean distance transform. Each foreground
// pixel in the thresholded mask seeds the JFA; subsequent passes propagate
// nearest-seed UVs across the texture in O(log N) passes. The final pass
// converts nearest-seed UV to a normalised distance value.

import { standardVertexSource, type NodeRenderContext, type NodeTextureRenderer } from "../../gl/noderenderer.js";
import { TextureNode } from "../../node.js";
import type { BoolProp, FloatProp } from "../../props.js";

interface DistanceTransformRenderData {
    threshold: number;
    spread: number;
    invert: boolean;
}

class DistanceTransformRenderer implements NodeTextureRenderer<DistanceTransformRenderData> {
    render(ctx: NodeRenderContext, data: DistanceTransformRenderData): void {
        const gl = ctx.gl;
        const cache = ctx.cache;
        const w = ctx.textureWidth;
        const h = ctx.textureHeight;

        if (ctx.inputs.length === 0) {
            ctx.clearOutput(0, 0, 0, 1);
            return;
        }

        const seedShader = cache.getOrCompileShader("jfadt_seed", standardVertexSource, seedFrag);
        const jfaShader = cache.getOrCompileShader("jfadt_step", standardVertexSource, jfaFrag);
        const distShader = cache.getOrCompileShader("jfadt_dist", standardVertexSource, distFrag);

        let texA = cache.acquireTexture(w, h);
        let texB = cache.acquireTexture(w, h);

        // --- Seed pass: foreground pixels write their own UV; background writes (-1,-1) ---
        cache.bindFboToTexture(texA);
        ctx.useShader(seedShader);
        ctx.bindTexture(seedShader, "u_mask", ctx.inputs[0]!.texture, 0);
        gl.uniform1f(gl.getUniformLocation(seedShader, "u_threshold"), data.threshold);
        ctx.drawQuad();

        // --- JFA passes ---
        const maxDim = Math.max(w, h);
        let stepSize = Math.trunc(maxDim / 2);
        while (stepSize >= 1) {
            cache.bindFboToTexture(texB);
            ctx.useShader(jfaShader);
            ctx.bindTexture(jfaShader, "u_input", texA, 0);
            gl.uniform1i(gl.getUniformLocation(jfaShader, "u_step"), stepSize);
            ctx.drawQuad();
            [texA, texB] = [texB, texA];
            stepSize = Math.trunc(stepSize / 2);
        }

        // --- Distance pass: convert nearest-seed UV to normalised distance ---
        cache.bindFboToTexture(ctx.outputTexture);
        ctx.useShader(distShader);
        ctx.bindTexture(distShader, "u_jfa", texA, 0);
        gl.uniform1f(gl.getUniformLocation(distShader, "u_spread"), data.spread);
        gl.uniform1i(gl.getUniformLocation(distShader, "u_invert"), data.invert ? 1 : 0);
        ctx.drawQuad();

        cache.releaseTexture(texA);
        cache.releaseTexture(texB);
    }
}

// Seed pass: store UV in RG if foreground, sentinel (-1,-1) if background
const seedFrag = /* glsl */ `
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
        `;

// JFA step: for each pixel, sample 8 neighbours at ±stepSize and keep
// whichever carries the nearest valid seed UV
const jfaFrag = /* glsl */ `
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
        `;

// Distance pass: convert nearest-seed UV to a normalised, spread-scaled
// grayscale value
const distFrag = /* glsl */ `
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
        `;

export class DistanceTransformNode extends TextureNode {
    override init(): void {
        this.title = "Distance Transform";
        this.addInput("mask");
        this.addFloatProp("threshold", "Threshold", 0.5, 0.0, 1.0, 0.01);
        this.addFloatProp("spread", "Spread", 0.5, 0.0, 1.0, 0.01);
        this.addBoolProp("invert", "Invert", false);
    }

    override createRenderer(): NodeTextureRenderer<DistanceTransformRenderData> {
        return new DistanceTransformRenderer();
    }

    override createRenderData(): DistanceTransformRenderData {
        const threshold = this.getProp("threshold") as FloatProp | undefined;
        const spread = this.getProp("spread") as FloatProp | undefined;
        const invert = this.getProp("invert") as BoolProp | undefined;
        return {
            threshold: threshold ? Math.fround(threshold.value) : 0.5,
            spread: spread ? Math.fround(spread.value) : 0.5,
            invert: invert ? invert.value : false,
        };
    }
}
