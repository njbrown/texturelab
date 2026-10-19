// Ported from src/texturelab/libraries/v2/floodfill.cpp.
// Keep the algorithm and props in sync with the C++ when either changes.
//
// CPU node. Output per island pixel: (u, v) within the island's bounding box,
// and the bbox size relative to the texture. Background = (0, 0, 0, 0).

import type { NodeRenderContext } from "../../gl/noderenderer.js";
import { TextureNode } from "../../node.js";
import { floodFillIslands, type IntensityFn } from "../floodfill.js";

// (r + g + b) * (1.0f / 3.0f) * ONE_OVER_VALUE_MAX, in float32 like the C++.
const ONE_THIRD = Math.fround(1 / 3);
const intensity: IntensityFn = (p, i) => Math.fround(Math.fround(Math.fround(p[i]! + p[i + 1]!) + p[i + 2]!) * ONE_THIRD);

export class FloodFillNode extends TextureNode {
    override init(): void {
        this.title = "Flood Fill";
        this.addInput("image");
        this.usesCpuProcessing = true;
        this.setShaderSource(/* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);
            return col;
        }
		`);
    }

    override cpuProcess(ctx: NodeRenderContext): void {
        const width = ctx.textureWidth;
        const height = ctx.textureHeight;
        if (width <= 0 || height <= 0) return;

        if (ctx.inputs.length === 0) {
            ctx.clearOutput(0, 0, 0, 0);
            return;
        }

        const readPixels = ctx.cache.readPixelsFloat(ctx.inputs[0]!.texture, width, height);

        // Process flood fill
        const threshold = Math.fround(0.1);
        const rects = floodFillIslands(readPixels, width, height, threshold, intensity);

        // Render output
        const results = new Float32Array(width * height * 4);
        for (const rect of rects) {
            const sx = Math.fround(rect.width * Math.fround(1 / width));
            const sy = Math.fround(rect.height * Math.fround(1 / height));
            const px = rect.pixels;
            for (let i = 0; i < px.length; i += 4) {
                const u = Math.fround((px[i + 2]! - rect.left) / rect.width);
                const v = Math.fround((px[i + 3]! - rect.top) / rect.height);
                const idx = 4 * (width * px[i + 1]! + px[i]!);
                results[idx + 0] = u;
                results[idx + 1] = v;
                results[idx + 2] = sx;
                results[idx + 3] = sy;
            }
        }

        // Upload result to texture (LINEAR + REPEAT, like the C++).
        const gl = ctx.gl;
        ctx.cache.uploadPixelsFloat(ctx.outputTexture, width, height, results);
        gl.bindTexture(gl.TEXTURE_2D, ctx.outputTexture);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
        gl.bindTexture(gl.TEXTURE_2D, null);
    }
}
