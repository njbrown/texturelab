// Ported from src/texturelab/libraries/v3/floodfillv2.cpp.
// Keep the algorithm and props in sync with the C++ when either changes.
//
// CPU BFS flood fill via NodeTextureRenderer.
//
// Output encoding (float RGBA per pixel):
//   R = island origin x (UV, wrapped to [0,1], quantized to texel center)
//   G = island origin y (same)
//   B = bbox width / texture width
//   A = bbox height / texture height
//   Background = (0, 0, 0, 0)

import type { NodeRenderContext, NodeTextureRenderer } from "../../gl/noderenderer.js";
import { TextureNode } from "../../node.js";
import type { FloatProp } from "../../props.js";
import { floodFillIslands } from "../floodfill.js";

interface FloodFillV2RenderData {
    threshold: number;
}

class FloodFillV2Renderer implements NodeTextureRenderer<FloodFillV2RenderData> {
    render(ctx: NodeRenderContext, data: FloodFillV2RenderData): void {
        const gl = ctx.gl;
        const cache = ctx.cache;
        const w = ctx.textureWidth;
        const h = ctx.textureHeight;

        if (w <= 0 || h <= 0) return;

        // No input — output black
        if (ctx.inputs.length === 0) {
            ctx.clearOutput(0, 0, 0, 1);
            return;
        }

        // Read input pixels via FBO
        const readPixels = cache.readPixelsFloat(ctx.inputs[0]!.texture, w, h);

        const islands = floodFillIslands(readPixels, w, h, data.threshold);

        // Build output texture
        // Encoding: (origin_x, origin_y, bbox_w, bbox_h)
        // origin is the top-left of the bbox, wrapped to [0,1] UV space,
        // quantized to texel centers for consistency across all pixels
        const results = new Float32Array(w * h * 4);
        const invW = Math.fround(1 / w);
        const invH = Math.fround(1 / h);

        for (const island of islands) {
            const bboxW = Math.fround(island.width * invW);
            const bboxH = Math.fround(island.height * invH);

            // Origin in UV space, wrapped to [0,1] and quantized to texel center
            const originLocalX = wrapAround(island.left, w);
            const originLocalY = wrapAround(island.top, h);
            const originU = Math.fround((originLocalX + 0.5) * invW);
            const originV = Math.fround((originLocalY + 0.5) * invH);

            const px = island.pixels;
            for (let i = 0; i < px.length; i += 4) {
                const idx = 4 * (w * px[i + 1]! + px[i]!);
                results[idx + 0] = originU;
                results[idx + 1] = originV;
                results[idx + 2] = bboxW;
                results[idx + 3] = bboxH;
            }
        }

        // Upload result to output texture (LINEAR + REPEAT, like the C++).
        cache.uploadPixelsFloat(ctx.outputTexture, w, h, results);
        gl.bindTexture(gl.TEXTURE_2D, ctx.outputTexture);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
        gl.bindTexture(gl.TEXTURE_2D, null);
    }
}

const wrapAround = (value: number, bound: number) => ((value % bound) + bound) % bound;

export class FloodFillV2Node extends TextureNode {
    override init(): void {
        this.title = "Flood Fill V2";
        this.addInput("image");
        this.addFloatProp("threshold", "Threshold", 0.1, 0.0, 1.0, 0.01);
        this.setShaderSource(/* glsl */ `
        vec4 process(vec2 uv)
        {
            return texture(image, uv);
        }
    `);
    }

    override createRenderer(): NodeTextureRenderer<FloodFillV2RenderData> {
        return new FloodFillV2Renderer();
    }

    override createRenderData(): FloodFillV2RenderData {
        const threshProp = this.getProp("threshold") as FloatProp | undefined;
        return { threshold: threshProp ? Math.fround(threshProp.value) : 0.1 };
    }
}
