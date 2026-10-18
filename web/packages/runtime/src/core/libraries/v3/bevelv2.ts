// Ported from src/texturelab/libraries/v3/bevelv2.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { MultiPassNode } from "../../node.js";

// TODO(port): the C++ node renders with a custom multi-pass
// NodeTextureRenderer, which isn't ported yet. init() is ported so files
// containing this node load and report it as unsupported.

export class BevelV2Node extends MultiPassNode {
    override init(): void {
        this.title = "Bevel V2";
        this.addInput("image");
        this.addFloatProp("distance", "Distance", 50.0, 0.0, 200.0, 0.5);
        this.addFloatProp("threshold", "Threshold", 0.5, 0.0, 1.0, 0.01);
        this.addEnumProp("shape", "Shape", ["Linear", "Round", "Smooth"]);
        this.addBoolProp("invert", "Invert", false);
        this.addBoolProp("scaleInvariant", "Scale Invariant", true);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            return texture(image, uv);
        }
    `;
        this.setShaderSource(source);
    }
}
