// Ported from src/texturelab/libraries/v2/skew.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class SkewNode extends TextureNode {
    override init(): void {
        this.title = "Skew";
        this.addInput("image");
        this.addEnumProp("axis", "Axis", ["Horizontal", "Vertical"]);
        this.addFloatProp("skew", "skew", 0.5, -1.0, 1.0, 0.01);
        this.addFloatProp("position", "Position", 0.5, 0.0, 1.0, 0.01);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            float xSkew = 0.0;
            float ySkew = 0.0;
            vec2 offset = vec2(0.0, 0.0);
            if (prop_axis == 0) {
                offset = vec2(0.0, prop_position);
                xSkew = prop_skew;
            }
            else {
                offset = vec2(prop_position, 0.0);
                ySkew = prop_skew;
            }

            mat3 skewMat = mat3(1.0, ySkew, 0.0,
                                xSkew, 1.0, 0.0,
                                0.0, 0.0, 1.0);
            
            uv -= offset;
            uv = (skewMat * vec3(uv, 1.0)).xy;
            uv += offset;

            // Time varying pixel color
            vec4 col =  texture(image, uv);
            return col;
        }
        `;
        this.setShaderSource(source);
    }
}
