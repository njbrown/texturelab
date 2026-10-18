// Ported from src/texturelab/libraries/v2/histogramshift.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class HistogramShiftNode extends TextureNode {
    override init(): void {
        this.title = "Histogram Shift";
        this.addInput("image");
        this.addFloatProp("position", "Position", 0.0, 0.0, 1.0, 0.01);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);

            // shows artifacts for pixels at 1 when position is at 0
            a.rgb = mod(a.rgb + vec3(prop_position), vec3(1.0));

            return a;
        }
        `;
        this.setShaderSource(source);
    }
}
