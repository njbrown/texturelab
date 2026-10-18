// Ported from src/texturelab/libraries/v2/histogramscan.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class HistogramScanNode extends TextureNode {
    override init(): void {
        this.title = "Histogram Scan";
        this.addInput("image");
        this.addFloatProp("position", "Position", 0.5, 0.0, 1.0, 0.01);
        this.addFloatProp("contrast", "Contrast", 0.01, 0.01, 1.0, 0.01);
        this.addBoolProp("invert", "Invert", false);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);

            vec3 col = a.rgb;
    
            vec3 diff = abs(col - vec3(prop_position));
            diff = clamp(diff, 0.0, prop_contrast);
            
            vec3 result = diff * (1.0 / prop_contrast);

            if(prop_invert)
                return vec4(result, a.a);
            else
                return vec4(vec3(1.0) - result, a.a);
        }
        `;
        this.setShaderSource(source);
    }
}
