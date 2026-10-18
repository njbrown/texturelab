// Ported from src/texturelab/libraries/v2/invertnormal.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class InvertNormalNode extends TextureNode {
    override init(): void {
        this.title = "Invert Normal";
        this.addInput("inputImage");
        this.addBoolProp("invertRed", "Invert Red", false);
        this.addBoolProp("invertGreen", "Invert Green", true);
        this.addBoolProp("invertBlue", "Invert Blue", false);
        this.addBoolProp("invertAlpha", "Invert Alpha", false);
        const source = /* glsl */ `

        vec4 process(vec2 uv)
        {
            // grayscale input color
            vec4 result = texture(inputImage,uv);

			if(prop_invertRed) result.r = 1.0 - result.r;
			if(prop_invertGreen) result.g = 1.0 - result.g;
			if(prop_invertBlue) result.b = 1.0 - result.b;
			if(prop_invertAlpha) result.a = 1.0 - result.a;
            
            return result;
        }
          `;
        this.setShaderSource(source);
    }
}
