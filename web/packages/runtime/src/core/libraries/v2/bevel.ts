// Ported from src/texturelab/libraries/v2/bevel.cpp.
// Keep the algorithm and props in sync with the C++ when either changes.
//
// CPU node: signed Euclidean distance transform of the input's red channel,
// turned into a normalised bevel ramp. Math.fround marks where the C++ works
// in float rather than double.

import type { NodeRenderContext } from "../../gl/noderenderer.js";
import { TextureNode } from "../../node.js";
import type { FloatProp } from "../../props.js";

// Constants for Euclidean Distance Transform
const INF = 1e20;
const VALUE_MAX = 1.0;

export class BevelNode extends TextureNode {
    override init(): void {
        this.title = "Bevel";
        this.addInput("image");
        this.addFloatProp("distance", "Distance", 50.0, 0.0, 100.0, 0.01);

        // This node uses CPU processing instead of GPU shader
        this.usesCpuProcessing = true;

        // Passthrough shader (not used)
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

        const gridSize = width * height;
        const readPixels = ctx.cache.readPixelsFloat(ctx.inputs[0]!.texture, width, height);
        const resultPixels = new Float32Array(gridSize * 4);

        // Allocate working arrays
        const maxSize = Math.max(width, height);
        const f = new Float64Array(maxSize * 3);
        const z = new Float64Array(maxSize * 3 + 1);
        const v = new Int32Array(maxSize * 3);

        const gridOuter = new Float64Array(gridSize);
        const gridInner = new Float64Array(gridSize);
        const grid = new Float64Array(gridSize);

        // Convert pixels to distance fields
        for (let i = 0; i < gridSize; i++) {
            const a = readPixels[i * 4 + 0]!; // Use red channel
            const outer = Math.max(0, Math.fround(0.5 - a));
            const inner = Math.max(0, Math.fround(a - 0.5));
            gridOuter[i] = a === 1 ? 0 : a === 0 ? INF : outer * outer;
            gridInner[i] = a === 1 ? INF : a === 0 ? 0 : inner * inner;
        }

        // Apply Euclidean Distance Transform
        edt(gridOuter, width, height, f, v, z);
        edt(gridInner, width, height, f, v, z);

        const radiusProp = this.getProp("distance") as FloatProp | undefined;
        const radius = radiusProp ? Math.fround(radiusProp.value) : 50;
        const offset = 0.25;

        // Calculate bevel
        let minVal = 1.0;
        let maxVal = 0.0;

        for (let i = 0; i < gridSize; i++) {
            const d = Math.sqrt(gridOuter[i]!) - Math.sqrt(gridInner[i]!);
            let col = Math.fround(VALUE_MAX - VALUE_MAX * (d / radius + offset));
            col = Math.max(0, Math.min(VALUE_MAX, col));

            minVal = Math.min(minVal, col);
            maxVal = Math.max(maxVal, col);
            grid[i] = col;
        }

        // Normalize and invert
        const range = Math.fround(maxVal - minVal);
        const scale = range > 0 ? Math.fround(1 / range) : 1;

        for (let i = 0; i < gridSize; i++) {
            const col = Math.fround(1 - (grid[i]! - minVal) * scale); // de-invert
            resultPixels[i * 4 + 0] = col;
            resultPixels[i * 4 + 1] = col;
            resultPixels[i * 4 + 2] = col;
            resultPixels[i * 4 + 3] = 1;
        }

        // Upload result to texture (LINEAR + REPEAT, like the C++).
        const gl = ctx.gl;
        ctx.cache.uploadPixelsFloat(ctx.outputTexture, width, height, resultPixels);
        gl.bindTexture(gl.TEXTURE_2D, ctx.outputTexture);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
        gl.bindTexture(gl.TEXTURE_2D, null);
    }
}

// 2D Euclidean squared distance transform by Felzenszwalb & Huttenlocher
// https://cs.brown.edu/~pff/papers/dt-final.pdf
function edt(data: Float64Array, width: number, height: number, f: Float64Array, v: Int32Array, z: Float64Array): void {
    for (let x = 0; x < width; x++) edt1d(data, x, width, height, f, v, z);
    for (let y = 0; y < height; y++) edt1d(data, y * width, 1, width, f, v, z);
}

// 1D squared distance transform
function edt1d(
    grid: Float64Array,
    offset: number,
    stride: number,
    length: number,
    f: Float64Array,
    v: Int32Array,
    z: Float64Array,
): void {
    v[0] = 0;
    z[0] = -INF;
    z[1] = INF;

    // Load line in array three times for wrapping
    for (let q = 0; q < length; q++) f[q] = grid[offset + q * stride]!;
    for (let q = 0; q < length; q++) f[q + length] = grid[offset + q * stride]!;
    for (let q = 0; q < length; q++) f[q + length + length] = grid[offset + q * stride]!;

    let k = 0;
    for (let q = 1; q < length * 3; q++) {
        let s: number;
        do {
            const r = v[k]!;
            s = (f[q]! - f[r]! + q * q - r * r) / (q - r) / 2.0;
        } while (s <= z[k]! && --k > -1);

        k++;
        v[k] = q;
        z[k] = s;
        z[k + 1] = INF;
    }

    // Copy over middle section
    for (let q = length, k2 = 0; q < length + length; q++) {
        while (z[k2 + 1]! < q) k2++;
        const r = v[k2]!;
        grid[offset + (q - length) * stride] = f[r]! + (q - r) * (q - r);
    }
}
