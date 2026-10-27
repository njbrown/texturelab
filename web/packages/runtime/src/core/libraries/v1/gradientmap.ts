// Ported from src/texturelab/libraries/v1/gradientmap.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";
import { Gradient } from "../../gradient.js";

export class GradientMapNode extends TextureNode {
    override init(): void {
        this.title = "GradientMap";
        this.addInput("inputImage");
        this.addGradientProp("gradient", "Gradient", Gradient.defaultGradient());
        const source = /* glsl */ `
        float grayscale(vec3 col)
        {
            return (col.r + col.g + col.b) / 3.0;
        }

        vec4 process(vec2 uv)
        {
            // grayscale input color
            float t = grayscale(texture(inputImage, uv).rgb);
            vec3 col = sampleGradient(prop_gradient, t);

            return vec4(col, 1.0);
        }
        `;
        this.setShaderSource(source);
    }
}
