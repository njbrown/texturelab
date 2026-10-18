// Ported from src/texturelab/libraries/v3/autolevels.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { MultiPassNode } from "../../node.js";

// TODO(port): the C++ node renders with a custom multi-pass
// NodeTextureRenderer, which isn't ported yet. init() is ported so files
// containing this node load and report it as unsupported.

export class AutoLevelsNode extends MultiPassNode {
    override init(): void {
        this.title = "Auto Levels";
        this.addInput("image");
        this.addFloatProp("gamma", "Midpoint Gamma", 1.0, 0.1, 4.0, 0.05);
        this.addBoolProp ("per_channel", "Per Channel", false);
    }
}
