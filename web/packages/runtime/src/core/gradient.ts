// Mirrors src/colorpicker/gradient.h.

import { colorFromJson, QColor, type Color } from "./color.js";
import { getFloat, isObject } from "./json.js";

export interface GradientPoint {
    position: number;
    color: Color;
}

export class Gradient {
    // Must match GRADIENT_MAX_POINTS in the shader prelude: stops beyond it
    // aren't rendered.
    static readonly MaxPoints = 32;

    points: GradientPoint[] = [];

    constructor(points: GradientPoint[] = []) {
        for (const p of points) this.addPoint(p);
    }

    /** Returns false (and adds nothing) once the gradient is full. */
    addPoint(point: GradientPoint): boolean {
        if (this.points.length >= Gradient.MaxPoints) return false;
        this.points.push({ position: point.position, color: { ...point.color } });
        return true;
    }

    clone(): Gradient {
        return new Gradient(this.points);
    }

    static defaultGradient(): Gradient {
        return new Gradient([
            { position: 0, color: QColor(0, 0, 0, 255) },
            { position: 1, color: QColor(255, 255, 255, 255) },
        ]);
    }

    // Like GradientProp::fromJsonValue, points are kept in file order (not
    // sorted) — the desktop app doesn't sort on load either.
    static fromJson(val: unknown): Gradient {
        const gradient = new Gradient();
        if (!isObject(val) || !Array.isArray(val.points)) return gradient;
        for (const p of val.points) {
            if (!isObject(p)) continue;
            gradient.addPoint({
                position: getFloat(p.t),
                color: colorFromJson(p.color),
            });
        }
        return gradient;
    }
}
