// Texture variables: project-level values that drive node props. Mirrors
// src/texturelab/variables.h/.cpp — same file format (the .texture
// "variables" array, Electron 0.3 field names) and the same tolerant
// validation. See docs/texture-variables-plan.md.

import { colorFromJson, type Color } from "./color.js";
import { getBool, getDouble, getString, getTruncatedInt, isObject } from "./json.js";
import type { TextureNode } from "./node.js";
import type { AnyProp } from "./props.js";

export type VariableType = "float" | "int" | "bool" | "enum" | "color";
export type VariableValue = number | boolean | Color;

// Numeric types in the file (Electron's DesignerVariableType).
const TYPE_FROM_NUMBER: Record<number, VariableType> = { 1: "float", 2: "int", 3: "bool", 4: "enum", 5: "color" };

export interface VariableLink {
    nodeId: string;
    propName: string;
}

export interface EnumOption {
    /** Index into the linked props' own option list. */
    index: number;
    label: string;
}

export interface TextureVariable {
    id: string;
    /** Identifier: unique, [A-Za-z_][A-Za-z0-9_]*. */
    name: string;
    displayName: string;
    type: VariableType;
    value: VariableValue;
    /** The value in the file; resetVariables() goes back to it. */
    defaultValue: VariableValue;
    /** float/int bounds. */
    min?: number;
    max?: number;
    step?: number;
    /** enum: the props' full option list. */
    values?: string[];
    /** enum: the options to offer, with their labels (all of them when the file doesn't restrict). */
    options?: EnumOption[];
    links: VariableLink[];
}

/** An identifier derived from a display name (variables::sanitizeName). */
export function sanitizeName(displayName: string): string {
    let name = displayName
        .trim()
        .toLowerCase()
        .replace(/[^a-z0-9]+/g, "_")
        .replace(/^_+|_+$/g, "");
    if (!name) name = "variable";
    if (/^[0-9]/.test(name)) name = "_" + name;
    return name;
}

function uniqueName(base: string, taken: Set<string>): string {
    if (!taken.has(base)) return base;
    for (let i = 2; ; i++) if (!taken.has(`${base}_${i}`)) return `${base}_${i}`;
}

const PROP_TYPE_FOR: Record<VariableType, AnyProp["type"]> = {
    float: "float",
    int: "int",
    bool: "bool",
    enum: "enum",
    color: "color",
};

/** Whether `prop` can link to `variable`: same type, and for enums the same option list. */
export function accepts(variable: TextureVariable, prop: AnyProp | undefined): boolean {
    if (!prop || prop.type !== PROP_TYPE_FOR[variable.type]) return false;
    if (prop.type === "enum") {
        const values = variable.values ?? [];
        return prop.values.length === values.length && prop.values.every((v, i) => v === values[i]);
    }
    return true;
}

/** Clamp a value to the variable's bounds / enabled options. */
export function clampValue(variable: TextureVariable, value: VariableValue): VariableValue {
    switch (variable.type) {
        case "float":
        case "int": {
            let v = typeof value === "number" ? value : Number(value);
            if (!Number.isFinite(v)) v = variable.value as number;
            if (variable.type === "int") v = Math.trunc(v);
            if (variable.min !== undefined && variable.max !== undefined && variable.min <= variable.max) {
                v = Math.min(variable.max, Math.max(variable.min, v));
            }
            return v;
        }
        case "bool":
            return Boolean(value);
        case "enum": {
            const count = variable.values?.length ?? 0;
            if (count === 0) return 0;
            let v = Math.min(count - 1, Math.max(0, Math.trunc(Number(value) || 0)));
            const options = variable.options ?? [];
            if (options.length && !options.some((o) => o.index === v)) v = options[0]!.index;
            return v;
        }
        case "color": {
            const c = (isObject(value) ? value : {}) as Partial<Color>;
            const ch = (x: unknown, d: number) => (typeof x === "number" && Number.isFinite(x) ? x : d);
            return { r: ch(c.r, 0), g: ch(c.g, 0), b: ch(c.b, 0), a: ch(c.a, 1) };
        }
    }
}

/** Write a variable's value into one prop. */
function writeTo(variable: TextureVariable, prop: AnyProp): void {
    if (!accepts(variable, prop)) return;
    switch (prop.type) {
        case "float":
        case "int":
            prop.value = variable.value as number;
            break;
        case "bool":
            prop.value = variable.value as boolean;
            break;
        case "enum":
            prop.index = variable.value as number;
            break;
        case "color":
            prop.value = { ...(variable.value as Color) };
            break;
    }
}

function parseOne(obj: Record<string, unknown>): TextureVariable | null {
    const type = TYPE_FROM_NUMBER[getTruncatedInt(obj.type, 0)];
    if (!type) return null;

    const prop = isObject(obj.property) ? obj.property : {};
    const name = getString(prop.name);
    const variable: TextureVariable = {
        id: getString(obj.id) || cryptoId(),
        name,
        displayName: getString(prop.displayName) || name,
        type,
        value: 0,
        defaultValue: 0,
        links: [],
    };

    switch (type) {
        case "float":
            variable.value = getDouble(prop.value, 0);
            variable.min = getDouble(prop.minValue, 0);
            variable.max = getDouble(prop.maxValue, 100);
            variable.step = getDouble(prop.step, 1);
            break;
        case "int":
            variable.value = getTruncatedInt(prop.value, 0);
            variable.min = getTruncatedInt(prop.minValue, 0);
            variable.max = getTruncatedInt(prop.maxValue, 100);
            variable.step = getTruncatedInt(prop.step, 1);
            break;
        case "bool":
            variable.value = getBool(prop.value, false);
            break;
        case "enum":
            variable.value = getTruncatedInt(prop.index, 0);
            variable.values = Array.isArray(prop.values) ? prop.values.map((v) => getString(v)) : [];
            variable.options = Array.isArray(obj.options)
                ? obj.options.filter(isObject).map((o) => ({ index: getTruncatedInt(o.index), label: getString(o.label) }))
                : [];
            break;
        case "color":
            variable.value = colorFromJson(prop.value);
            break;
    }

    if (Array.isArray(obj.linkedProperties)) {
        for (const l of obj.linkedProperties) {
            if (!isObject(l)) continue;
            const link = { nodeId: getString(l.nodeId), propName: getString(l.name) };
            if (link.nodeId && link.propName && !variable.links.some((x) => x.nodeId === link.nodeId && x.propName === link.propName)) {
                variable.links.push(link);
            }
        }
    }
    return variable;
}

let idCounter = 0;
function cryptoId(): string {
    return globalThis.crypto?.randomUUID?.() ?? `var-${Date.now()}-${idCounter++}`;
}

/**
 * Parse the file's `variables` array and make it consistent with the loaded
 * nodes, like variables::validate(): drop links to missing/mismatched props
 * and to props already linked, make names valid and unique, drop bad enum
 * options, clamp values. Then apply every variable to its props.
 */
export function loadVariables(json: unknown, nodes: Map<string, TextureNode>): TextureVariable[] {
    const parsed = (Array.isArray(json) ? json : []).filter(isObject).map(parseOne);
    const variables: TextureVariable[] = [];
    const ids = new Set<string>();
    const names = new Set<string>();
    const linked = new Set<string>();

    for (const variable of parsed) {
        if (!variable) continue;

        if (ids.has(variable.id)) variable.id = cryptoId();
        ids.add(variable.id);

        let name = /^[A-Za-z_][A-Za-z0-9_]*$/.test(variable.name)
            ? variable.name
            : sanitizeName(variable.name || variable.displayName);
        name = uniqueName(name, names);
        variable.name = name;
        names.add(name);
        if (!variable.displayName) variable.displayName = name;

        if (variable.type === "enum") {
            const values = variable.values ?? [];
            const seen = new Set<number>();
            const valid: EnumOption[] = [];
            for (const option of variable.options ?? []) {
                if (option.index < 0 || option.index >= values.length || seen.has(option.index)) continue;
                seen.add(option.index);
                valid.push({ index: option.index, label: option.label || values[option.index]! });
            }
            variable.options = valid;
        }

        variable.links = variable.links.filter((link) => {
            const key = `${link.nodeId}/${link.propName}`;
            if (linked.has(key)) return false;
            if (!accepts(variable, nodes.get(link.nodeId)?.getProp(link.propName))) return false;
            linked.add(key);
            return true;
        });

        variable.value = clampValue(variable, variable.value);
        variable.defaultValue = copyValue(variable.value);

        // Resolve "no restriction" to the full option list for consumers.
        if (variable.type === "enum" && variable.options!.length === 0) {
            variable.options = (variable.values ?? []).map((label, index) => ({ index, label }));
        }

        applyVariable(variable, nodes);
        variables.push(variable);
    }
    return variables;
}

function copyValue(value: VariableValue): VariableValue {
    return typeof value === "object" ? { ...value } : value;
}

/** Write the variable's value into every linked prop. Returns the ids of the nodes it changed. */
export function applyVariable(variable: TextureVariable, nodes: Map<string, TextureNode>): string[] {
    const changed: string[] = [];
    for (const link of variable.links) {
        const node = nodes.get(link.nodeId);
        const prop = node?.getProp(link.propName);
        if (!node || !prop) continue;
        writeTo(variable, prop);
        changed.push(node.id);
    }
    return changed;
}
