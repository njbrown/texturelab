#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// Classic 2D gradient (Perlin) noise, upgraded for v3:
//   - Quintic smooth step: C2-continuous, no gradient discontinuities at cell
//   edges
//   - Normalized pseudo-random gradients via hash22 → unit circle
//   - Seamless tiling: integer grid + mod()-wrapped cell indices
//   - fBm with per-octave lacunarity / gain controls
//   - Three output modes: Standard, Ridged, Billowy
//
// References:
//   Perlin, "An Image Synthesizer," SIGGRAPH 1985
//   https://iquilezles.org/articles/gradientnoise/ — gradient noise primer
void PerlinNoiseNode::init()
{
    this->title = "Perlin Noise";

    auto modeProp = this->addEnumProp("output_mode", "Output",
                                      {"Standard", "Ridged", "Billowy"});
    modeProp->index = 0;

    this->addIntProp("scale", "Scale", 4, 1, 12, 1);
    this->addIntProp("octaves", "Octaves", 6, 1, 8, 1);
    this->addFloatProp("lacunarity", "Lacunarity", 2.0, 1.5, 4.0, 0.1);
    this->addFloatProp("gain", "Gain", 0.5, 0.2, 0.8, 0.05);

    auto source = R""""(
        #define OUT_STANDARD 0
        #define OUT_RIDGED   1
        #define OUT_BILLOWY  2

        // 2D gradient noise.
        // gridSize must be a positive integer so that mod() wrapping produces
        // seamless tiling: cell (gridSize) hashes identically to cell 0.
        float gnoise(vec2 uv, float gridSize) {
            vec2 p = uv * gridSize;
            vec2 i = floor(p);
            vec2 f = fract(p);

            // Quintic interpolant: 6t^5 - 15t^4 + 10t^3 (zero first & second derivative at 0 and 1)
            vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

            vec2 sOff  = vec2(_seed * 0.173, _seed * 0.251);
            vec2 gs    = vec2(gridSize);

            // Unit-circle gradients from hash22 (maps [0,1]^2 → [-1,1]^2, then normalize)
            vec2 g00 = normalize(-1.0 + 2.0 * hash22(mod(i,              gs) + sOff));
            vec2 g10 = normalize(-1.0 + 2.0 * hash22(mod(i + vec2(1,0), gs) + sOff));
            vec2 g01 = normalize(-1.0 + 2.0 * hash22(mod(i + vec2(0,1), gs) + sOff));
            vec2 g11 = normalize(-1.0 + 2.0 * hash22(mod(i + vec2(1,1), gs) + sOff));

            // Dot gradient with distance-to-corner vector
            float n00 = dot(g00, f);
            float n10 = dot(g10, f - vec2(1,0));
            float n01 = dot(g01, f - vec2(0,1));
            float n11 = dot(g11, f - vec2(1,1));

            // Scale ~1/sqrt(0.5) ≈ 1.41 to stretch typical ±0.7 output to ±1
            return 1.41 * mix(mix(n00, n10, u.x), mix(n01, n11, u.x), u.y);
        }

        vec4 process(vec2 uv) {
            float value  = 0.0;
            float amp    = 0.5;
            float freq   = float(prop_scale);
            float maxAmp = 0.0;

            for (int i = 0; i < prop_octaves; i++) {
                // Round to integer so every octave grid tiles exactly
                float gs = max(1.0, floor(freq + 0.5));
                float n  = gnoise(uv, gs); // in approximately [-1, 1]

                if (prop_output_mode == OUT_RIDGED)
                    n = 1.0 - abs(n); // sharp ridges at zero-crossings
                else if (prop_output_mode == OUT_BILLOWY)
                    n = abs(n);       // rounded bumps everywhere

                value  += amp * n;
                maxAmp += amp;
                freq   *= prop_lacunarity;
                amp    *= prop_gain;
            }

            float f = value / maxAmp;

            float result;
            if (prop_output_mode == OUT_STANDARD)
                result = clamp(0.5 + 0.5 * f, 0.0, 1.0);
            else
                result = clamp(f, 0.0, 1.0);

            return vec4(vec3(result), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
