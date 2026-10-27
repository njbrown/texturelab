// Ported from src/texturelab/libraries/v3/bevelv2.cpp.
// Keep the GLSL and props in sync with the C++ when either changes.
//
// JFA-based GPU bevel. One deliberate difference: the C++ never releases its
// two ping-pong textures back to the pool; this port does.

import { standardVertexSource, type NodeRenderContext, type NodeTextureRenderer } from "../../gl/noderenderer.js";
import { TextureNode } from "../../node.js";
import type { BoolProp, EnumProp, FloatProp } from "../../props.js";

interface BevelV2RenderData {
    distance: number;
    threshold: number;
    shape: number; // 0=Linear, 1=Round, 2=Smooth
    invert: boolean;
    scaleInvariant: boolean;
}

class BevelV2Renderer implements NodeTextureRenderer<BevelV2RenderData> {
    render(ctx: NodeRenderContext, data: BevelV2RenderData): void {
        const gl = ctx.gl;
        const cache = ctx.cache;
        const w = ctx.textureWidth;
        const h = ctx.textureHeight;

        // No input connected — output black
        if (ctx.inputs.length === 0) {
            ctx.clearOutput(0, 0, 0, 1);
            return;
        }

        // Compile shaders (cached after first call)
        const seedShader = cache.getOrCompileShader("jfa_seed", standardVertexSource, seedFrag);
        const jfaShader = cache.getOrCompileShader("jfa_step", standardVertexSource, jfaFrag);
        const bevelShader = cache.getOrCompileShader("jfa_bevel", standardVertexSource, bevelFrag);

        // Acquire two intermediate textures for ping-pong
        let texA = cache.acquireTexture(w, h);
        let texB = cache.acquireTexture(w, h);

        // --- Seed initialization ---
        cache.bindFboToTexture(texA);
        ctx.useShader(seedShader);
        ctx.bindTexture(seedShader, "image", ctx.inputs[0]!.texture, 0);
        gl.uniform1f(gl.getUniformLocation(seedShader, "u_threshold"), data.threshold);
        gl.uniform1i(gl.getUniformLocation(seedShader, "u_invert"), data.invert ? 1 : 0);
        ctx.drawQuad();

        // --- JFA iteration (ping-pong) ---
        const maxDim = Math.max(w, h);
        let stepSize = Math.trunc(maxDim / 2);

        while (stepSize >= 1) {
            cache.bindFboToTexture(texB);
            ctx.useShader(jfaShader);
            ctx.bindTexture(jfaShader, "u_input", texA, 0);
            gl.uniform1i(gl.getUniformLocation(jfaShader, "u_stepSize"), stepSize);
            ctx.drawQuad();

            [texA, texB] = [texB, texA];
            stepSize = Math.trunc(stepSize / 2);
        }

        // --- Final pass: Distance field → bevel height ---
        cache.bindFboToTexture(ctx.outputTexture);
        ctx.useShader(bevelShader);
        ctx.bindTexture(bevelShader, "u_jfa", texA, 0);
        gl.uniform1f(gl.getUniformLocation(bevelShader, "u_distance"), data.distance);
        gl.uniform1i(gl.getUniformLocation(bevelShader, "u_shape"), data.shape);
        gl.uniform1i(gl.getUniformLocation(bevelShader, "u_invert"), data.invert ? 1 : 0);
        gl.uniform1i(gl.getUniformLocation(bevelShader, "u_scaleInvariant"), data.scaleInvariant ? 1 : 0);
        ctx.drawQuad();

        cache.releaseTexture(texA);
        cache.releaseTexture(texB);
    }
}

const seedFrag = /* glsl */ `
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
                // flat. \`invert\` swaps which side is treated as the seed
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
        `;

const jfaFrag = /* glsl */ `
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
        `;

const bevelFrag = /* glsl */ `
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
                // was tuned against). Disabled, \`distance\` is read as a
                // raw pixel count instead — matching the old (v1/v2) Bevel
                // node, which always operated in absolute pixel terms.
                float pixelDistance = (u_scaleInvariant == 1)
                    ? u_distance * (max(_textureSize.x, _textureSize.y) / 512.0)
                    : u_distance;
                // Distance 0 would be 0/0 on the seeds themselves.
                float t = clamp(dist / max(pixelDistance, 1e-4), 0.0, 1.0);

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
        `;

export class BevelV2Node extends TextureNode {
    override init(): void {
        this.title = "Bevel V2";
        this.addInput("image");
        this.addFloatProp("distance", "Distance", 50.0, 0.0, 200.0, 0.5);
        this.addFloatProp("threshold", "Threshold", 0.5, 0.0, 1.0, 0.01);
        this.addEnumProp("shape", "Shape", ["Linear", "Round", "Smooth"]);
        this.addBoolProp("invert", "Invert", false);
        this.addBoolProp("scaleInvariant", "Scale Invariant", true);

        // Passthrough shader (not used during rendering — the custom renderer
        // handles all passes)
        this.setShaderSource(/* glsl */ `
        vec4 process(vec2 uv)
        {
            return texture(image, uv);
        }
    `);
    }

    override createRenderer(): NodeTextureRenderer<BevelV2RenderData> {
        return new BevelV2Renderer();
    }

    override createRenderData(): BevelV2RenderData {
        const distance = this.getProp("distance") as FloatProp | undefined;
        const threshold = this.getProp("threshold") as FloatProp | undefined;
        const shape = this.getProp("shape") as EnumProp | undefined;
        const invert = this.getProp("invert") as BoolProp | undefined;
        const scaleInvariant = this.getProp("scaleInvariant") as BoolProp | undefined;
        return {
            distance: distance ? Math.fround(distance.value) : 50.0,
            threshold: threshold ? Math.fround(threshold.value) : 0.5,
            shape: shape ? shape.index : 0,
            invert: invert ? invert.value : false,
            scaleInvariant: scaleInvariant ? scaleInvariant.value : true,
        };
    }
}
