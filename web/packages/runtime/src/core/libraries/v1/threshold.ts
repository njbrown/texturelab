// Ported from src/texturelab/libraries/v1/threshold.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class ThresholdNode extends TextureNode {
    override init(): void {
        this.title = "Threshold";
        this.addInput("image");
        this.addFloatProp("threshold", "Threshold", 0.0, 0.0, 1.0, 0.01);
        this.addBoolProp("invert", "Invert", true);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);

            if (prop_invert)
                a.rgb = step(1.0 - prop_threshold, a.rgb);
            else
                a.rgb = step(prop_threshold, a.rgb);

            return a;
        }
        `;
        this.setShaderSource(source);
    }
}
