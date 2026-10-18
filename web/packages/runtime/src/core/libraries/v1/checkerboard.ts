// Ported from src/texturelab/libraries/v1/checkerboard.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";
import { QColor } from "../../color.js";

export class CheckerboardNode extends TextureNode {
    override init(): void {
        this.title = "Checkerboard";
        this.addFloatProp("rows", "Rows", 2, 1, 20, 1);
        this.addFloatProp("columns", "Columns", 2, 1, 20, 1);
        this.addColorProp("color", "Color", QColor());
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            if ((mod(prop_columns*uv.x, 1.0) < 0.5) ^^ (mod(prop_rows*uv.y, 1.0) < 0.5))
            {
                return vec4(prop_color.rgb, 1.0);
            }
            else
            {
                return vec4(1.0, 1.0, 1.0, 1.0);
            }
        }
        `;
        this.setShaderSource(source);
    }
}
