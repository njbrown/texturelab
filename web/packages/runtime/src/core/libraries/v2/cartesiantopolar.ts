// Ported from src/texturelab/libraries/v2/cartesiantopolar.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class CartesianToPolarNode extends TextureNode {
    override init(): void {
        this.title = "Cartesian To Polar";
        this.addInput("image");
        this.addFloatProp("yscale", "Y Scale", 1.0, 0, 4.0, 0.01);
        this.addIntProp("xtile", "X Tile", 2, 0, 5, 1);
        this.addFloatProp("angle_offset", "Angle Offset", 0.0, 0, 360, 1.0);
        const source = /* glsl */ `
        const float PI = 3.142;
        vec4 process(vec2 uv)
        {
            vec2 dir = uv - vec2(0.5, 0.5);
            float y = length(dir) * 2.0 * prop_yscale;

            float x = atan(dir.y, dir.x);
            x = (x + PI) / (PI * 2.0); // bring to range 0..1
            x += (prop_angle_offset / 360.0);
            x = x * float(prop_xtile);

            vec4 col = texture(image, vec2(x, y));

            return col;
        }
          `;
        this.setShaderSource(source);
    }
}
