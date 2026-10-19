// Ported from src/texturelab/libraries/v3/blurhq.cpp.
// Keep the GLSL and props in sync with the C++ when either changes.

// https://learnopengl.com/Advanced-Lighting/Bloom (separated Gaussian in GLSL)
// Wells, "Efficient Synthesis of Gaussian Filters by Cascaded Uniform Filters,"
//   IEEE PAMI 1986
// True separated Gaussian blur: two 1D passes (H then V) give O(2N) samples
// vs O(N²) for a 2D kernel — making large-radius blurs practical on the GPU.

import {
    standardVertexSource,
    withTextureParams,
    type NodeRenderContext,
    type NodeTextureRenderer,
} from "../../gl/noderenderer.js";
import { TextureNode } from "../../node.js";
import type { FloatProp } from "../../props.js";

interface BlurHQRenderData {
    radius: number;
}

class BlurHQRenderer implements NodeTextureRenderer<BlurHQRenderData> {
    render(ctx: NodeRenderContext, data: BlurHQRenderData): void {
        const gl = ctx.gl;
        const cache = ctx.cache;
        const w = ctx.textureWidth;
        const h = ctx.textureHeight;

        // No input — output black
        if (ctx.inputs.length === 0) {
            ctx.clearOutput(0, 0, 0, 1);
            return;
        }

        const hShader = cache.getOrCompileShader("blurhq_horizontal", standardVertexSource, horizontalFrag());
        const vShader = cache.getOrCompileShader("blurhq_vertical", standardVertexSource, verticalFrag());

        const intermediate = cache.acquireTexture(w, h);
        const inputTex = ctx.inputs[0]!.texture;

        // GL_LINEAR is required for the bilinear tap trick in the shaders —
        // sampling at fractional offsets must interpolate rather than snap.
        withTextureParams(
            gl,
            [
                { texture: intermediate, minFilter: gl.LINEAR, magFilter: gl.LINEAR },
                { texture: inputTex, minFilter: gl.LINEAR, magFilter: gl.LINEAR },
            ],
            () => {
                // --- Pass 1: horizontal blur ---
                cache.bindFboToTexture(intermediate);
                ctx.useShader(hShader);
                ctx.bindTexture(hShader, "u_image", inputTex, 0);
                gl.uniform1f(gl.getUniformLocation(hShader, "u_radius"), data.radius);
                gl.uniform2f(gl.getUniformLocation(hShader, "_textureSize"), w, h);
                ctx.drawQuad();

                // --- Pass 2: vertical blur ---
                cache.bindFboToTexture(ctx.outputTexture);
                ctx.useShader(vShader);
                ctx.bindTexture(vShader, "u_image", intermediate, 0);
                gl.uniform1f(gl.getUniformLocation(vShader, "u_radius"), data.radius);
                gl.uniform2f(gl.getUniformLocation(vShader, "_textureSize"), w, h);
                ctx.drawQuad();
            },
        );

        cache.releaseTexture(intermediate);
    }
}

// Shared Gaussian sampling code — axis is injected per-pass.
//
// Uses the bilinear tap trick: instead of sampling at each integer pixel
// offset i, adjacent taps i and i+1 are collapsed into a single fetch at
// the Gaussian-weighted midpoint between them.
function gaussianBody(axis: string): string {
    return /* glsl */ `
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

                    vec2 o = ${axis} * offset * step;
                    result += texture(u_image, fract(uv + o)) * w;
                    result += texture(u_image, fract(uv - o)) * w;
                    totalW += 2.0 * w;
                }

                fragColor = result / max(totalW, 0.0001);
            }
        `;
}

const horizontalFrag = () => gaussianBody("vec2(1.0, 0.0)");
const verticalFrag = () => gaussianBody("vec2(0.0, 1.0)");

export class BlurHQNode extends TextureNode {
    override init(): void {
        this.title = "Blur HQ";
        this.addInput("image");
        this.addFloatProp("radius", "Radius", 8.0, 0.5, 128.0, 0.5);
    }

    override createRenderer(): NodeTextureRenderer<BlurHQRenderData> {
        return new BlurHQRenderer();
    }

    override createRenderData(): BlurHQRenderData {
        const prop = this.getProp("radius") as FloatProp | undefined;
        return { radius: prop ? Math.fround(prop.value) : 8.0 };
    }
}
