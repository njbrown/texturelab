// Ported from src/texturelab/libraries/v3/floodfillv2tocolor.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class FloodFillV2ToColorNode extends TextureNode {
    override init(): void {
        this.title = "FF To Color V2";
        this.addInput("floodfill");
        this.addInput("color");
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 data = texture(floodfill, uv);

            // Background check: bbox size is zero
            if (data.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);

            // Origin is stored directly in RG — no reconstruction needed
            vec2 origin = data.rg;

            return texture(color, origin);
        }
    `;
        this.setShaderSource(source);
    }
}
