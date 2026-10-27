// Ported from src/texturelab/libraries/v2/pow.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class PowNode extends TextureNode {
    override init(): void {
        this.title = "Pow";
        this.addInput("image");
        this.addFloatProp("exponent", "Exponent", 1.0, 0.0, 10.0, 0.01);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);
            a.rgb = pow(a.rgb, vec3(prop_exponent));

            return a;
        }
        `;
        this.setShaderSource(source);
    }
}
