// Ported from src/texturelab/libraries/v1/brightnesscontrast.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class BrightnessContrastNode extends TextureNode {
    override init(): void {
        this.title = "Brightness Contrast";
        this.addInput("image");
        this.addFloatProp("contrast", "Contrast", 0.0, -1, 1, 0.1);
        this.addFloatProp("brightness", "Brightness", 0.0, -1, 1, 0.1);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);

            col.rgb += prop_brightness;
            if (prop_contrast > 0.0)
                col.rgb = (col.rgb - 0.5) / (1.0 - prop_contrast) + 0.5;
            else
                col.rgb = (col.rgb - 0.5) * (1.0 + prop_contrast) + 0.5;

            return col;
        }
        `;
        this.setShaderSource(source);
    }
}
