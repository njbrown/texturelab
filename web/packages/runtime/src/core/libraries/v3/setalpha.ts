// Ported from src/texturelab/libraries/v3/setalpha.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class SetAlphaNode extends TextureNode {
    override init(): void {
        this.title = "Set Alpha";
        this.addInput("rgba");
        this.addInput("alpha");
        const prop = this.addEnumProp( "alphaChannel", "Alpha Channel", ["Red", "Green", "Blue", "Alpha", "Average (RGB)"]);
        prop.setValue(0);
        this.addBoolProp("invert", "Invert Alpha", false);
        const source = /* glsl */ `
        float getChannel(vec4 inputData, int mode)
        {
            if (mode == 0) return inputData.r;
            if (mode == 1) return inputData.g;
            if (mode == 2) return inputData.b;
            if (mode == 3) return inputData.a;
            if (mode == 4) {
                return (inputData.r + inputData.g + inputData.b) * 0.3333333;
            }

            return 0.0;
        }

        vec4 process(vec2 uv)
        {
            vec3 rgb = rgba_connected ? texture(rgba, uv).rgb : vec3(0.0);

            float a = 1.0;
            if (alpha_connected) {
                a = getChannel(texture(alpha, uv), prop_alphaChannel);
                if (prop_invert) a = 1.0 - a;
            }

            return vec4(rgb, a);
        }
    `;
        this.setShaderSource(source);
    }
}
