// Ported from src/texturelab/libraries/v2/gradientdynamic.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class GradientDynamicNode extends TextureNode {
    override init(): void {
        this.title = "Gradient Dynamic";
        this.addInput("image");
        this.addInput("gradient");
        this.addFloatProp("position", "Position", 0, 0, 1, 0.01);
        this.addEnumProp("orientation", "Gradient Direction", ["Vertical", "Horizontal"]);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            float t = 0.0;
    
            float intensity = texture(image, uv).r;

            vec4 col = vec4(0);
            if (prop_orientation == 0)
                col = texture(gradient, vec2(prop_position, intensity));
            else
                col = texture(gradient, vec2(intensity, prop_position));
            
            return col;
        }
        `;
        this.setShaderSource(source);
    }
}
