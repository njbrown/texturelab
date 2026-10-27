// Mirrors src/texturelab/jsonutils.h.
//
// Older project files aren't strict about json types: numbers were sometimes
// written as strings ("1.5", "3"), ints as floats (3.0) and bools as
// "true"/"false". These helpers coerce whatever is in the json into the type
// the caller wants and fall back to defaultValue when the value is missing or
// can't be read as one.

export function getDouble(val: unknown, defaultValue = 0): number {
    if (typeof val === "number") return Number.isFinite(val) ? val : defaultValue;

    if (typeof val === "string") {
        const trimmed = val.trim();
        if (trimmed === "") return defaultValue;
        const num = Number(trimmed);
        return Number.isFinite(num) ? num : defaultValue;
    }

    if (typeof val === "boolean") return val ? 1 : 0;

    return defaultValue;
}

export function getFloat(val: unknown, defaultValue = 0): number {
    return Math.fround(getDouble(val, defaultValue));
}

// C++ uses llround: halves round away from zero.
export function getLong(val: unknown, defaultValue = 0): number {
    const d = getDouble(val, defaultValue);
    return d < 0 ? -Math.round(-d) : Math.round(d);
}

export function getInt(val: unknown, defaultValue = 0): number {
    return getLong(val, defaultValue) | 0;
}

/**
 * Int props and enum indices. Fractional values only occur in files saved by
 * the old Electron app, which passed them straight to gl.uniform1i — and that
 * truncates. Truncating here keeps those files looking the way they were
 * authored (Grenade.texture's polygon has sides: "4.72" and should have 4).
 *
 * Deliberately differs from jsonutils::getLong, which rounds (since ba1deac),
 * and so renders such files differently from how they were made.
 */
export function getTruncatedInt(val: unknown, defaultValue = 0): number {
    return Math.trunc(getDouble(val, defaultValue)) | 0;
}

export function getBool(val: unknown, defaultValue = false): boolean {
    if (typeof val === "boolean") return val;

    if (typeof val === "number") return val !== 0;

    if (typeof val === "string") {
        const str = val.trim().toLowerCase();
        if (str === "true" || str === "yes") return true;
        if (str === "false" || str === "no" || str === "") return false;
        return getDouble(val, defaultValue ? 1 : 0) !== 0;
    }

    return defaultValue;
}

export function getString(val: unknown, defaultValue = ""): string {
    if (typeof val === "string") return val;
    if (typeof val === "number") return String(val);
    if (typeof val === "boolean") return val ? "true" : "false";
    return defaultValue;
}

export function isObject(val: unknown): val is Record<string, unknown> {
    return typeof val === "object" && val !== null && !Array.isArray(val);
}
