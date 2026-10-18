// Ported from src/texturelab/libraries/v3/rays.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class RaysNode extends TextureNode {
    override init(): void {
        this.title = "Rays";
        this.addIntProp("sides", "Sides", 3, 1, 32, 1);
        this.addFloatProp("angle", "Angle", 0.0, 0.0, 360.0, 1.0);
        this.addFloatProp("translateX", "Translate X", 0.5, 0.0, 1.0, 0.01);
        this.addFloatProp("translateY", "Translate Y", 0.5, 0.0, 1.0, 0.01);
        const source = /* glsl */ `
        #define PI 3.14159265359

        vec4 process(vec2 uv)
        {
            uv -= vec2(prop_translateX, prop_translateY);

            float a = atan(uv.x, uv.y) + radians(prop_angle);
            float shape = sin(a * float(prop_sides));

            return vec4(vec3(shape), 1.0);
        }
    `;
        this.setShaderSource(source);
    }
}
