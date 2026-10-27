import { getFloat, isObject } from "./json.js";

/** RGBA color with float components in 0..1, like QColor's redF()/greenF()/... */
export interface Color {
    r: number;
    g: number;
    b: number;
    a: number;
}

/**
 * Stand-in for Qt's QColor constructor so ported node code reads the same as
 * the C++: QColor(r, g, b[, a]) takes 0..255 ints. QColor() (an invalid color
 * in Qt) reads back as opaque black.
 */
export function QColor(r = 0, g = 0, b = 0, a = 255): Color {
    return { r: r / 255, g: g / 255, b: b / 255, a: a / 255 };
}

// Like ColorProp::fromJsonValue: missing components read as 0 (alpha as 1).
export function colorFromJson(val: unknown): Color {
    const obj: Record<string, unknown> = isObject(val) ? val : {};
    return {
        r: getFloat(obj.r),
        g: getFloat(obj.g),
        b: getFloat(obj.b),
        a: getFloat(obj.a, 1),
    };
}
