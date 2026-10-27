// Ported from src/texturelab/libraries/v3/fbmdomainwarp.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class FBMDomainWarpNode extends TextureNode {
    override init(): void {
        this.title = "FBM Domain Warp";
        this.addIntProp ("scale", "Scale", 3, 1, 12, 1);
        this.addIntProp ("octaves", "Octaves", 6, 1, 8, 1);
        this.addFloatProp("warp", "Primary Warp", 1.0, 0.0, 4.0, 0.1);
        this.addFloatProp("warp2", "Secondary Warp", 0.5, 0.0, 4.0, 0.1);
        this.addBoolProp ("two_pass", "Double Warp", true);
        const source = /* glsl */ `
        // Tileable value noise: cell hashes use mod()-wrapped indices so the
        // noise repeats exactly every gridSize cells along each axis.
        float vnoise(vec2 sampleUV, float gridSize) {
            vec2 i = floor(sampleUV);
            vec2 f = fract(sampleUV);
            vec2 u = f * f * (3.0 - 2.0 * f);

            vec2 sOff = vec2(_seed * 0.173);

            float a = hash12(mod(i + vec2(0.0, 0.0), vec2(gridSize)) + sOff);
            float b = hash12(mod(i + vec2(1.0, 0.0), vec2(gridSize)) + sOff);
            float c = hash12(mod(i + vec2(0.0, 1.0), vec2(gridSize)) + sOff);
            float d = hash12(mod(i + vec2(1.0, 1.0), vec2(gridSize)) + sOff);

            return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
        }

        // Multi-octave fBm.  Each octave doubles the grid size (lacunarity 2);
        // since baseScale is integer, every octave's grid size remains integer
        // → mod() wrapping is exact and the entire fBm is seamlessly tileable.
        float fbm(vec2 uv) {
            float value    = 0.0;
            float amp      = 0.5;
            float gridSize = float(prop_scale);
            for (int i = 0; i < prop_octaves; i++) {
                value    += amp * vnoise(uv * gridSize, gridSize);
                gridSize *= 2.0;
                amp      *= 0.5;
            }
            return value;
        }

        vec4 process(vec2 uv)
        {
            // Warp pass 1: offset UV by two independent fBm samples.
            // The constant offsets (5.2, 1.3) etc. just shift which part of
            // the noise we read; they don't affect tileability because mod()
            // wrapping handles any sample position.
            vec2 q = vec2(
                fbm(uv + vec2(0.000, 0.000)),
                fbm(uv + vec2(5.200, 1.300))
            );

            float f;
            if (prop_two_pass) {
                // Warp pass 2: use q to offset again (recursive swirling).
                // Since q is itself seamless, q at uv=0 equals q at uv=1,
                // so the warped sampling positions still differ by exactly 1
                // across the tile boundary — fbm's mod() wrap maps them to
                // the same noise value.
                vec2 r = vec2(
                    fbm(uv + prop_warp * q + vec2(1.700, 9.200)),
                    fbm(uv + prop_warp * q + vec2(8.300, 2.800))
                );
                f = fbm(uv + prop_warp2 * r);
            } else {
                f = fbm(uv + prop_warp * q);
            }

            // Remap to [0,1] and add a slight contrast lift
            f = clamp(f * 1.4 - 0.1, 0.0, 1.0);

            return vec4(vec3(f), 1.0);
        }
    `;
        this.setShaderSource(source);
    }
}
