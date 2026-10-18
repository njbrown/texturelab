// Ported from src/texturelab/libraries/v3/floodfillv2.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { MultiPassNode } from "../../node.js";

// TODO(port): the C++ node renders with a custom multi-pass
// NodeTextureRenderer, which isn't ported yet. init() is ported so files
// containing this node load and report it as unsupported.

export class FloodFillV2Node extends MultiPassNode {
    override init(): void {
        this.title = "Flood Fill V2";
        this.addInput("image");
        this.addFloatProp("threshold", "Threshold", 0.1, 0.0, 1.0, 0.01);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            return texture(image, uv);
        }
    `;
        this.setShaderSource(source);
    }
}
