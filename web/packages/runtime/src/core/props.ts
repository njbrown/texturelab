// Mirrors src/texturelab/props.h. Each prop knows how to read its value from a
// .texture file's `properties` object (fromJsonValue) the same way the desktop
// app does, including the lenient legacy coercions.

import { colorFromJson, QColor, type Color } from "./color.js";
import { Curve } from "./curve.js";
import { Gradient } from "./gradient.js";
import { getBool, getDouble, getInt, getLong, getString } from "./json.js";

export type PropType =
    | "float"
    | "int"
    | "bool"
    | "color"
    | "enum"
    | "string"
    | "gradient"
    | "image"
    | "curve";

export abstract class Prop {
    abstract readonly type: PropType;
    name = "";
    displayName = "";
    order = 0;
    group: PropertyGroup | null = null;

    abstract getValue(): unknown;
    abstract setValue(val: unknown): void;
    abstract fromJsonValue(val: unknown): void;
}

export class PropertyGroup {
    name = "";
    props: Prop[] = [];
    collapsed = true;

    add(prop: Prop): void {
        this.props.push(prop);
        prop.group = this;
    }
}

export class FloatProp extends Prop {
    readonly type = "float";
    value = 0;
    minValue = 0;
    maxValue = 100;
    step = 1;

    getValue() {
        return this.value;
    }
    setValue(val: unknown) {
        this.value = getDouble(val, this.value);
    }
    fromJsonValue(val: unknown) {
        this.value = getDouble(val, this.value);
    }
}

export class IntProp extends Prop {
    readonly type = "int";
    value = 0;
    minValue = 0;
    maxValue = 100;
    step = 1;

    getValue() {
        return this.value;
    }
    setValue(val: unknown) {
        this.value = Math.trunc(getDouble(val, this.value));
    }
    fromJsonValue(val: unknown) {
        this.value = getLong(val, this.value);
    }
}

export class BoolProp extends Prop {
    readonly type = "bool";
    value = false;

    getValue() {
        return this.value;
    }
    setValue(val: unknown) {
        this.value = getBool(val, this.value);
    }
    fromJsonValue(val: unknown) {
        this.value = getBool(val, this.value);
    }
}

export class EnumProp extends Prop {
    readonly type = "enum";
    values: string[] = [];
    index = 0;

    getValue() {
        return this.index;
    }
    setValue(val: unknown) {
        this.index = getInt(val, this.index);
    }
    fromJsonValue(val: unknown) {
        this.index = getInt(val, this.index);
    }
}

export class ColorProp extends Prop {
    readonly type = "color";
    value: Color = QColor();

    getValue() {
        return this.value;
    }
    setValue(val: unknown) {
        this.value = colorFromJson(val);
    }
    fromJsonValue(val: unknown) {
        this.value = colorFromJson(val);
    }
}

export class StringProp extends Prop {
    readonly type = "string";
    value = "";

    getValue() {
        return this.value;
    }
    setValue(val: unknown) {
        this.value = getString(val, this.value);
    }
    fromJsonValue(val: unknown) {
        this.value = typeof val === "string" ? val : "";
    }
}

export class GradientProp extends Prop {
    readonly type = "gradient";
    value: Gradient = Gradient.defaultGradient();

    getValue() {
        return this.value;
    }
    setValue(val: unknown) {
        if (val instanceof Gradient) this.value = val.clone();
    }
    fromJsonValue(val: unknown) {
        this.value = Gradient.fromJson(val);
    }
}

export class CurveProp extends Prop {
    readonly type = "curve";
    value: Curve = new Curve();

    getValue() {
        return this.value;
    }
    setValue(val: unknown) {
        if (val instanceof Curve) this.value = val.clone();
    }
    fromJsonValue(val: unknown) {
        this.value = Curve.fromJson(val);
    }
}

/**
 * Holds the embedded image as its data URL. Decoding happens in the renderer
 * (asynchronously, via createImageBitmap), so parsing stays synchronous and
 * DOM-free.
 */
export class ImageProp extends Prop {
    readonly type = "image";
    /** `data:image/...;base64,...`, or "" when no image is set. */
    value = "";

    getValue() {
        return this.value;
    }
    setValue(val: unknown) {
        this.value = typeof val === "string" ? val : "";
    }
    fromJsonValue(val: unknown) {
        const str = typeof val === "string" ? val : "";
        this.value = str.includes(";base64,") ? str : "";
    }
}

export type AnyProp =
    | FloatProp
    | IntProp
    | BoolProp
    | EnumProp
    | ColorProp
    | StringProp
    | GradientProp
    | CurveProp
    | ImageProp;
