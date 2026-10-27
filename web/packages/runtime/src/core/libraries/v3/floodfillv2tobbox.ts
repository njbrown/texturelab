// Ported from src/texturelab/libraries/v3/floodfillv2tobbox.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class FloodFillV2ToBBoxNode extends TextureNode {
    override init(): void {
        this.title = "FF To BBox V2";
        this.addInput("floodfill");
        this.addEnumProp("function", "Function", ["max(x,y)", "min(x,y)", "x", "y", "length(x,y)"]);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 data = texture(floodfill, uv);

            if (data.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);

            // BA = bbox size (width, height) relative to texture
            float intensity = 0.0;

            if (prop_function == 0)
                intensity = max(data.b, data.a);
            else if (prop_function == 1)
                intensity = min(data.b, data.a);
            else if (prop_function == 2)
                intensity = data.b;
            else if (prop_function == 3)
                intensity = data.a;
            else if (prop_function == 4)
                intensity = sqrt(data.b * data.b + data.a * data.a);

            return vec4(vec3(intensity), 1.0);
        }
    `;
        this.setShaderSource(source);
    }
}
