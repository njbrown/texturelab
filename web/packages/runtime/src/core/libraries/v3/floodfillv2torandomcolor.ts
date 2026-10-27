// Ported from src/texturelab/libraries/v3/floodfillv2torandomcolor.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class FloodFillV2ToRandomColorNode extends TextureNode {
    override init(): void {
        this.title = "FF To Random Color V2";
        this.addInput("floodfill");
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 data = texture(floodfill, uv);

            if (data.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);

            // Origin stored directly — use as hash seed
            vec2 origin = data.rg;

            vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
            color.r = _rand(vec2(_seed) + origin + vec2(1) * vec2(0.01));
            color.g = _rand(vec2(_seed) + origin + vec2(2) * vec2(0.01));
            color.b = _rand(vec2(_seed) + origin + vec2(3) * vec2(0.01));

            return color;
        }
    `;
        this.setShaderSource(source);
    }
}
