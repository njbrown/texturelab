// Ported from src/texturelab/libraries/v1/wave.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class WaveNode extends TextureNode {
    override init(): void {
        this.title = "Wave";
        this.addIntProp("xfrequency", "X Frequency", 1, 0, 20, 1);
        this.addIntProp("yfrequency", "Y Frequency", 0, 0, 20, 1);
        this.addFloatProp("phase", "Phase Offset", 0.0, 0.0, 3.142, 0.01);
        this.addFloatProp("amp", "Amplitude", 0.5, 0.0, 1.0, 0.01);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            float fx = uv.x * 3.142 * 2.0 * float(prop_xfrequency);
            float fy = uv.y * 3.142 * 2.0 * float(prop_yfrequency);
            float wave = sin(fx + fy + prop_phase) * prop_amp;

            // bring wave to range 0...1
            wave = wave * 0.5 + 0.5;

            return vec4(vec3(wave), 1.0);
        }
        `;
        this.setShaderSource(source);
    }
}
