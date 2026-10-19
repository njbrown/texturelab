// BFS island search shared by v2 FloodFillNode::cpuProcess and v3
// FloodFillV2Renderer (src/texturelab/libraries/v2/floodfill.cpp,
// v3/floodfillv2.cpp), which both run the same loop.
//
// The C++ marks a pixel visited when it's popped, so a pixel can sit in the
// queue several times. Marking it when it's pushed instead visits pixels in
// the same order with the same global (unwrapped) coordinates — with a FIFO
// queue the first push of a pixel is also its first pop, and the skipped
// duplicates never push anything — while bounding the queue to one entry per
// pixel.

export interface Island {
    left: number;
    top: number;
    right: number;
    bottom: number;
    width: number; // right - left
    height: number; // bottom - top
    /** Flat [localX, localY, globalX, globalY, ...]. */
    pixels: number[];
}

/** Intensity of pixel i (RGBA float index i*4) using float32 arithmetic like the C++. */
export type IntensityFn = (pixels: Float32Array, i: number) => number;

// (r + g + b) / 3.0f — v3 FloodFillV2.
export const averageIntensity: IntensityFn = (p, i) =>
    Math.fround(Math.fround(Math.fround(p[i]! + p[i + 1]!) + p[i + 2]!) / 3);

const wrapAround = (value: number, bound: number) => ((value % bound) + bound) % bound;

/**
 * Islands of pixels whose intensity is >= threshold, 4-connected with
 * wrap-around. Islands with zero bbox width or height are dropped, as in the
 * C++.
 */
export function floodFillIslands(
    pixels: Float32Array,
    w: number,
    h: number,
    threshold: number,
    intensity: IntensityFn = averageIntensity,
): Island[] {
    const gridSize = w * h;
    const visited = new Uint8Array(gridSize);
    // Global coords can leave [0, w) when an island wraps, so store them as-is.
    const queueX = new Int32Array(gridSize);
    const queueY = new Int32Array(gridSize);
    const islands: Island[] = [];

    for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
            if (visited[y * w + x]) continue;

            const island: Island = {
                left: Infinity,
                top: Infinity,
                right: -Infinity,
                bottom: -Infinity,
                width: 0,
                height: 0,
                pixels: [],
            };
            let head = 0;
            let tail = 0;
            visited[y * w + x] = 1;
            queueX[tail] = x;
            queueY[tail++] = y;

            while (head < tail) {
                const gx = queueX[head]!;
                const gy = queueY[head++]!;
                const lx = wrapAround(gx, w);
                const ly = wrapAround(gy, h);

                if (intensity(pixels, 4 * (w * ly + lx)) < threshold) continue;

                if (gx < island.left) island.left = gx;
                if (gy < island.top) island.top = gy;
                if (gx > island.right) island.right = gx;
                if (gy > island.bottom) island.bottom = gy;
                island.pixels.push(lx, ly, gx, gy);

                // +x, -x, +y, -y — the C++ push order.
                const nx = [gx + 1, gx - 1, gx, gx];
                const ny = [gy, gy, gy + 1, gy - 1];
                for (let k = 0; k < 4; k++) {
                    const idx = wrapAround(ny[k]!, h) * w + wrapAround(nx[k]!, w);
                    if (visited[idx]) continue;
                    visited[idx] = 1;
                    queueX[tail] = nx[k]!;
                    queueY[tail++] = ny[k]!;
                }
            }

            island.width = island.right - island.left;
            island.height = island.bottom - island.top;
            if (island.width > 0 && island.height > 0) islands.push(island);
        }
    }

    return islands;
}
