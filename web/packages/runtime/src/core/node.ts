// Mirrors TextureNode in src/texturelab/models.h. Ported node classes extend
// this and fill it in from init(), using the same add*Prop calls as the C++.

import type { Color } from "./color.js";
import type { Gradient } from "./gradient.js";
import {
    BoolProp,
    ColorProp,
    CurveProp,
    EnumProp,
    FloatProp,
    GradientProp,
    ImageProp,
    IntProp,
    PropertyGroup,
    StringProp,
    type AnyProp,
} from "./props.js";

export class TextureNode {
    id = "";
    typeName = "";
    title = "";

    inputs: string[] = [];

    randomSeed = 0;
    exportName = "";

    props = new Map<string, AnyProp>();
    propertyGroups: PropertyGroup[] = [];

    shaderSource = "";

    // Set by nodes the desktop app renders on the CPU (v2 bevel/floodfill).
    usesCpuProcessing = false;

    /**
     * Non-null when this node needs a renderer the web runtime doesn't have
     * yet (multi-pass or CPU nodes). Evaluation reports these up front instead
     * of rendering something wrong.
     */
    get unsupportedReason(): string | null {
        return this.usesCpuProcessing ? "CPU-processed node is not ported to the web runtime yet" : null;
    }

    init(): void {}

    addInput(inputName: string): void {
        this.inputs.push(inputName);
    }

    setShaderSource(source: string): void {
        this.shaderSource = source;
    }

    getProp(name: string): AnyProp | undefined {
        return this.props.get(name);
    }

    hasProp(name: string): boolean {
        return this.props.has(name);
    }

    createGroup(name: string): PropertyGroup {
        const group = new PropertyGroup();
        group.name = name;
        this.propertyGroups.push(group);
        return group;
    }

    private register<T extends AnyProp>(prop: T, name: string, displayName: string): T {
        prop.name = name;
        prop.displayName = displayName;
        prop.order = this.props.size;
        this.props.set(name, prop);
        return prop;
    }

    // C++ declares the min/max/step params as ints; values are truncated.
    addIntProp(name: string, displayName: string, defaultVal = 1, minVal = 1, maxVal = 100, increment = 1): IntProp {
        const prop = new IntProp();
        prop.minValue = Math.trunc(minVal);
        prop.maxValue = Math.trunc(maxVal);
        prop.step = Math.trunc(increment);
        prop.value = Math.trunc(defaultVal);
        return this.register(prop, name, displayName);
    }

    addFloatProp(name: string, displayName: string, defaultVal = 1, minVal = 1, maxVal = 100, increment = 1): FloatProp {
        const prop = new FloatProp();
        prop.minValue = minVal;
        prop.maxValue = maxVal;
        prop.step = increment;
        prop.value = defaultVal;
        return this.register(prop, name, displayName);
    }

    addBoolProp(name: string, displayName: string, defaultVal = false): BoolProp {
        const prop = new BoolProp();
        prop.value = defaultVal;
        return this.register(prop, name, displayName);
    }

    addEnumProp(name: string, displayName: string, values: string[]): EnumProp {
        const prop = new EnumProp();
        prop.values = [...values];
        prop.index = 0;
        return this.register(prop, name, displayName);
    }

    addColorProp(name: string, displayName: string, defaultVal: Color): ColorProp {
        const prop = new ColorProp();
        prop.value = { ...defaultVal };
        return this.register(prop, name, displayName);
    }

    addStringProp(name: string, displayName: string, defaultVal = ""): StringProp {
        const prop = new StringProp();
        prop.value = defaultVal;
        return this.register(prop, name, displayName);
    }

    addGradientProp(name: string, displayName: string, defaultVal: Gradient): GradientProp {
        const prop = new GradientProp();
        prop.value = defaultVal.clone();
        return this.register(prop, name, displayName);
    }

    addImageProp(name: string, displayName: string): ImageProp {
        return this.register(new ImageProp(), name, displayName);
    }

    addCurveProp(name: string, displayName: string): CurveProp {
        return this.register(new CurveProp(), name, displayName);
    }
}

/**
 * Base for nodes the desktop app renders with a custom multi-pass
 * NodeTextureRenderer. Their init() (inputs/props) is ported so files load and
 * validate; the renderer itself is not ported yet.
 */
export class MultiPassNode extends TextureNode {
    override get unsupportedReason(): string | null {
        return "multi-pass node is not ported to the web runtime yet";
    }
}
