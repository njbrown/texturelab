// Mirrors src/texturelab/curve.h (the parts the renderer needs).

import { getBool, getFloat, isObject } from "./json.js";

// Must match CURVE_MAX_POINTS in the shader prelude.
export const CURVE_MAX_POINTS = 32;

export interface CurvePoint {
    x: number;
    y: number;
    lx: number; // left handle offset (lx <= 0)
    ly: number;
    rx: number; // right handle offset (rx >= 0)
    ry: number;
    smooth: boolean;
}

const clamp01 = (v: number) => Math.min(1, Math.max(0, v));

export class Curve {
    points: CurvePoint[];

    // Default: linear identity.
    constructor(points?: CurvePoint[]) {
        this.points = points
            ? points.map((p) => ({ ...p }))
            : [
                  { x: 0, y: 0, lx: -0.15, ly: -0.15, rx: 0.15, ry: 0.15, smooth: true },
                  { x: 1, y: 1, lx: -0.15, ly: -0.15, rx: 0.15, ry: 0.15, smooth: true },
              ];
    }

    clone(): Curve {
        return new Curve(this.points);
    }

    static fromJson(val: unknown): Curve {
        if (!isObject(val) || !Array.isArray(val.points) || val.points.length < 2) {
            return new Curve();
        }

        const points: CurvePoint[] = [];
        for (const o of val.points) {
            const p = isObject(o) ? o : {};
            points.push({
                x: clamp01(getFloat(p.x)),
                y: clamp01(getFloat(p.y)),
                lx: Math.min(getFloat(p.lx), 0),
                ly: getFloat(p.ly),
                rx: Math.max(getFloat(p.rx), 0),
                ry: getFloat(p.ry),
                smooth: getBool(p.smooth, true),
            });
        }

        // Sort by x, enforce first/last at 0/1 like Curve::fromJson.
        points.sort((a, b) => a.x - b.x);
        points[0]!.x = 0;
        points[points.length - 1]!.x = 1;

        return new Curve(points);
    }
}
