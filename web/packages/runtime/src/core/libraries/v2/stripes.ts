// Ported from src/texturelab/libraries/v2/stripes.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class StripesNode extends TextureNode {
    override init(): void {
        this.title = "Stripes";
        this.addIntProp("stripes", "Stripes", 10, 0, 30, 1);
        this.addIntProp("shift", "Shift", 0, 0, 30, 1);
        this.addFloatProp("width", "Width", 0.5, 0.0, 1.0, 0.01);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            float fx = uv.x * 3.142 * 2.0 * float(prop_stripes);
            float fy = uv.y * 3.142 * 2.0 * float(prop_shift);
            float wave = sin(fx + fy);

            // bring wave to range 0...1
            wave = wave * 0.5 + 0.5;
            wave = step(prop_width, wave);

            return vec4(vec3(wave), 1.0);
        }
        `;
        this.setShaderSource(source);
    }
}
