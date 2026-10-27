// Ported from src/texturelab/libraries/v1/heightshift.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class HeightShiftNode extends TextureNode {
    override init(): void {
        this.title = "Height Shift";
        this.addInput("image");
        this.addFloatProp("shift", "Shift", 0.0, -1.0, 1.0, 0.01);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);

            return a + vec4(vec3(prop_shift), 0.0);
        }
        `;
        this.setShaderSource(source);
    }
}
