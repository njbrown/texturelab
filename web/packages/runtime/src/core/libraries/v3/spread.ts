// Ported from src/texturelab/libraries/v3/spread.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { MultiPassNode } from "../../node.js";

// TODO(port): the C++ node renders with a custom multi-pass
// NodeTextureRenderer, which isn't ported yet. init() is ported so files
// containing this node load and report it as unsupported.

export class ColorSpreadNode extends MultiPassNode {
    override init(): void {
        this.title = "Spread";
        this.addInput("color");
        this.addInput("mask");
        this.addFloatProp("threshold", "Threshold", 0.5, 0.0, 1.0, 0.01);
        this.addFloatProp("distance", "Distance", 1.0, 0.0, 1.0, 0.01);
    }
}
