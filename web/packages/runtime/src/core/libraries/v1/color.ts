// Ported from src/texturelab/libraries/v1/color.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";
import { QColor } from "../../color.js";

export class ColorNode extends TextureNode {
    override init(): void {
        this.title = "Color";
        this.addColorProp("color", "Color", QColor(255, 255, 255, 255));
        this.setShaderSource(/* glsl */ `
        vec4 process(vec2 uv)
        {
            return prop_color;
        }
        `);
    }
}

export class ColorizeNode extends TextureNode {
    override init(): void {
        this.title = "Colorize";
        this.addInput("image");
        this.addColorProp("color", "Color", QColor(255, 255, 255, 255));
        this.setShaderSource(/* glsl */ `
        vec4 process(vec2 uv)
        {
            return texture(image,uv) * prop_color;
        }
        `);
    }
}
