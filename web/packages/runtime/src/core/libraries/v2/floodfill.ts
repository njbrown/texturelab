// Ported from src/texturelab/libraries/v2/floodfill.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class FloodFillNode extends TextureNode {
    override init(): void {
        this.title = "Flood Fill";
        this.addInput("image");
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
