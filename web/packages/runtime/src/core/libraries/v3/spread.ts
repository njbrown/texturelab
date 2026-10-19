// Ported from src/texturelab/libraries/v3/spread.cpp.
// Keep the GLSL and props in sync with the C++ when either changes.

// Color Spread — JFA nearest-seed lookup that samples a color map instead of
// computing a distance value. Each masked-out (black) pixel receives the exact
// color of the closest foreground pixel in the mask, producing solid filled
// regions with no gradient blending.

import { standardVertexSource, type NodeRenderContext, type NodeTextureRenderer } from "../../gl/noderenderer.js";
import { TextureNode } from "../../node.js";
import type { FloatProp } from "../../props.js";

interface ColorSpreadRenderData {
    threshold: number;
    distance: number;
}

class ColorSpreadRenderer implements NodeTextureRenderer<ColorSpreadRenderData> {
    render(ctx: NodeRenderContext, data: ColorSpreadRenderData): void {
        const gl = ctx.gl;
        const cache = ctx.cache;
        const w = ctx.textureWidth;
        const h = ctx.textureHeight;

        // Require the mask input; color map is optional (falls back to mask).
        const maskTex = ctx.inputTexture("mask");
        if (!maskTex) {
            ctx.clearOutput(0, 0, 0, 1);
            return;
        }

        const colorTex = ctx.inputTexture("color") ?? maskTex;

        const seedShader = cache.getOrCompileShader("cs_seed", standardVertexSource, seedFrag);
        const jfaShader = cache.getOrCompileShader("cs_jfa", standardVertexSource, jfaFrag);
        const sampleShader = cache.getOrCompileShader("cs_sample", standardVertexSource, sampleFrag);

        let texA = cache.acquireTexture(w, h);
        let texB = cache.acquireTexture(w, h);

        // Seed pass: foreground pixels (mask >= threshold) write their UV;
        // background pixels write the sentinel (-1, -1).
        cache.bindFboToTexture(texA);
        ctx.useShader(seedShader);
        ctx.bindTexture(seedShader, "u_mask", maskTex, 0);
        gl.uniform1f(gl.getUniformLocation(seedShader, "u_threshold"), data.threshold);
        ctx.drawQuad();

        // JFA passes: propagate nearest-seed UV across the texture
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

        // Sample pass: look up the color map at the nearest-seed UV
        cache.bindFboToTexture(ctx.outputTexture);
        ctx.useShader(sampleShader);
        ctx.bindTexture(sampleShader, "u_jfa", texA, 0);
        ctx.bindTexture(sampleShader, "u_color", colorTex, 1);
        gl.uniform1f(gl.getUniformLocation(sampleShader, "u_distance"), data.distance);
        ctx.drawQuad();

        cache.releaseTexture(texA);
        cache.releaseTexture(texB);
    }
}

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
                    fragColor = vec4(v_texCoord, 0.0, 1.0);
                else
                    fragColor = vec4(-1.0, -1.0, 0.0, 1.0);
            }
        `;

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
                        vec2 sampleUV = v_texCoord + vec2(float(x), float(y)) * step;
                        vec4 s        = texture(u_input, sampleUV);
                        vec2 seedUV   = s.rg;
                        if (seedUV.x < 0.0) continue;
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

// Sample the color map at the nearest-seed UV for a solid, gradient-free fill
const sampleFrag = /* glsl */ `
            #version 150
            in  vec2 v_texCoord;
            out vec4 fragColor;

            uniform sampler2D u_jfa;
            uniform sampler2D u_color;
            uniform float     u_distance;

            void main()
            {
                vec2 seedUV = texture(u_jfa, v_texCoord).rg;

                if (seedUV.x < 0.0) {
                    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
                } else {
                    float d = length(v_texCoord - seedUV);
                    if (d > u_distance * 0.5)
                        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
                    else
                        fragColor = texture(u_color, seedUV);
                }
            }
        `;

export class ColorSpreadNode extends TextureNode {
    override init(): void {
        this.title = "Spread";
        this.addInput("color");
        this.addInput("mask");
        this.addFloatProp("threshold", "Threshold", 0.5, 0.0, 1.0, 0.01);
        this.addFloatProp("distance", "Distance", 1.0, 0.0, 1.0, 0.01);
    }

    override createRenderer(): NodeTextureRenderer<ColorSpreadRenderData> {
        return new ColorSpreadRenderer();
    }

    override createRenderData(): ColorSpreadRenderData {
        const threshold = this.getProp("threshold") as FloatProp | undefined;
        const distance = this.getProp("distance") as FloatProp | undefined;
        return {
            threshold: threshold ? Math.fround(threshold.value) : 0.5,
            distance: distance ? Math.fround(distance.value) : 1.0,
        };
    }
}
