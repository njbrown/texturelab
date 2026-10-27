// Ported from src/texturelab/libraries/v3/swirl.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class SwirlNode extends TextureNode {
    override init(): void {
        this.title = "Swirl";
        this.addInput("image");
        this.addFloatProp("radius", "Radius", 0.7, 0.0, 1.0, 0.01);
        this.addFloatProp("angle", "Angle", 90.0, 0.0, 360.0, 0.1);
        this.addFloatProp("centerX", "Center X", 0.5, 0.0, 1.0, 0.01);
        this.addFloatProp("centerY", "Center Y", 0.5, 0.0, 1.0, 0.01);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.0, 0.0, 0.0, 1.0);

            vec2 center = vec2(prop_centerX, prop_centerY);
            vec2 delta = uv - center;
            float len = length(delta);
            float effectAngle = radians(prop_angle);

            // smoothstep is undefined when its edges are equal (radius 0).
            float swirlAmount = prop_radius > 0.0
                ? effectAngle * smoothstep(prop_radius, 0.0, len)
                : 0.0;
            float a = atan(delta.y, delta.x) + swirlAmount;

            vec2 swirlUV = center + vec2(cos(a), sin(a)) * len;

            return texture(image, swirlUV);
        }
    `;
        this.setShaderSource(source);
    }
}
