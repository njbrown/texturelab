// Ported from src/texturelab/libraries/v3/solidcellv3.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class SolidCellV3Node extends TextureNode {
    override init(): void {
        this.title = "Solid Cell";
        this.addFloatProp("scale", "Scale", 30.0, 1.0, 256.0, 1.0);
        this.addFloatProp("scaleX", "Scale X", 1.0, 0.1, 10.0, 0.1);
        this.addFloatProp("scaleY", "Scale Y", 1.0, 0.1, 10.0, 0.1);
        this.addBoolProp ("invert", "Invert", false);
        this.addFloatProp("entropy", "Order", 0.0, 0.0, 1.0, 0.01);
        this.addFloatProp("intensity", "Intensity", 1.0, 0.0, 2.0, 0.01);
        this.setShaderSource(/* glsl */ `
        vec4 process(vec2 uv)
        {
            float gsX = max(1.0, floor(prop_scale * prop_scaleX + 0.5));
            float gsY = max(1.0, floor(prop_scale * prop_scaleY + 0.5));

            vec2 scaledUV = vec2(uv.x * gsX, uv.y * gsY);
            vec2 i_st = floor(scaledUV);
            vec2 f_st = fract(scaledUV);

            vec2 seedOff = vec2(_seed * 0.173, _seed * 0.251);
            float m_dist = 1e9;
            vec2 closestWrapped = vec2(0.0);

            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    vec2 neighbor = vec2(float(x), float(y));
                    vec2 wrapped  = mod(i_st + neighbor, vec2(gsX, gsY));
                    vec2 point    = hash22(wrapped + seedOff);
                    point = mix(point, vec2(0.5), prop_entropy);
                    vec2 diff = neighbor + point - f_st;
                    float dist = length(diff);
                    if (dist < m_dist) {
                        m_dist = dist;
                        closestWrapped = wrapped;
                    }
                }
            }

            // Use a second hash to generate per-cell color so it is independent
            // of the cell-point position hash used above.
            float cellColor = hash22(closestWrapped + vec2(_seed * 0.391, _seed * 0.617)).x;
            vec3 color = vec3(cellColor);
            if (prop_invert) color = 1.0 - color;
            return vec4(color * prop_intensity, 1.0);
        }
    `);
    }
}
