// Ported from src/texturelab/libraries/v3/heightblend.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class HeightBlendNode extends TextureNode {
    override init(): void {
        this.title = "Height Blend";
        this.addInput("color_a");
        this.addInput("color_b");
        this.addInput("height_a");
        this.addInput("height_b");
        this.addInput("mask");
        this.addFloatProp("blend", "Blend Factor", 0.5, 0.0, 1.0, 0.01);
        this.addFloatProp("edge_width", "Edge Width", 0.1, 0.0, 0.5, 0.01);
        this.addFloatProp("contrast", "Contrast", 1.0, 0.1, 4.0, 0.1);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            // Gather inputs; fall back gracefully if not connected
            vec4 colA = color_a_connected ? texture(color_a, uv) : vec4(0.2, 0.2, 0.2, 1.0);
            vec4 colB = color_b_connected ? texture(color_b, uv) : vec4(0.8, 0.8, 0.8, 1.0);

            float hA  = height_a_connected ? texture(height_a, uv).r : 0.5;
            float hB  = height_b_connected ? texture(height_b, uv).r : 0.5;
            float wearMask = mask_connected ? texture(mask, uv).r : prop_blend;

            // wearMask=1 → full top coat; wearMask=0 → fully worn to base
            float coverage = clamp(wearMask * prop_blend * 2.0, 0.0, 1.0);

            // Height-based cutoff: find where the two height fields "meet"
            float hAscaled = hA * (1.0 - coverage);
            float hBscaled = hB * coverage;
            float cutoff   = max(hAscaled, hBscaled);

            float ew = max(prop_edge_width, 0.001);

            // Per-pixel blend factor from height intersection
            float bFactor = smoothstep(cutoff - ew, cutoff + ew, hBscaled);

            // Apply contrast to the blend factor
            if (prop_contrast != 1.0) {
                bFactor = clamp(
                    (bFactor - 0.5) * prop_contrast + 0.5,
                    0.0, 1.0
                );
            }

            vec4 result = mix(colA, colB, bFactor);
            return result;
        }
    `;
        this.setShaderSource(source);
    }
}
