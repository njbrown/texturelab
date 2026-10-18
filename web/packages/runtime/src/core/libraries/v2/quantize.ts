// Ported from src/texturelab/libraries/v2/quantize.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class QuantizeNode extends TextureNode {
    override init(): void {
        this.title = "Quantize";
        this.addInput("image");
        this.addIntProp("steps", "Steps", 12, 2, 20, 1);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);
            a.rgb = floor(a.rgb * vec3(float(prop_steps))) / vec3(float(prop_steps));

            return a;
        }
        `;
        this.setShaderSource(source);
    }
}
