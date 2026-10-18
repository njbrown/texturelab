// Ported from src/texturelab/libraries/v2/bevel.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class BevelNode extends TextureNode {
    override init(): void {
        this.title = "Bevel";
        this.addInput("image");
        this.addFloatProp("distance", "Distance", 50.0, 0.0, 100.0, 0.01);
        this.usesCpuProcessing = true;
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);
            return col;
        }
		`;
        this.setShaderSource(source);
    }
}
