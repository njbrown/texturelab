// Ported from src/texturelab/libraries/v3/blurhq.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { MultiPassNode } from "../../node.js";

// TODO(port): the C++ node renders with a custom multi-pass
// NodeTextureRenderer, which isn't ported yet. init() is ported so files
// containing this node load and report it as unsupported.

export class BlurHQNode extends MultiPassNode {
    override init(): void {
        this.title = "Blur HQ";
        this.addInput("image");
        this.addFloatProp("radius", "Radius", 8.0, 0.5, 128.0, 0.5);
    }
}
