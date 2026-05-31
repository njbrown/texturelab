#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// 3D gradient (Perlin) noise for texture work.
//
// The core insight: sampling a 2D cross-section through a 3D noise volume
// gives patterns that a purely 2D noise cannot — specifically wood grain and
// marble veins, the two applications Perlin described in his original 1985
// paper.  The Z Slice prop lets you navigate through the volume; rotating
// the cut angle gives completely different grain/vein patterns from the same
// seed, just as cutting a log at different angles produces different grain.
//
// Tiling: XY cell indices are mod()-wrapped at the integer grid boundary so
// the noise repeats seamlessly.  Z is intentionally left free — tiling in Z
// would create visible repetition along the depth axis.
//
// Modes:
//   Standard — raw fBm, remapped to [0,1]
//   Ridged   — 1 - |n| per octave → sharp mountain-ridge crests
//   Billowy  — |n| per octave → rounded pillow-cloud bumps
//   Wood     — noise-distorted concentric rings (log cross-section)
//   Marble   — noise-distorted sine stripes (classic Perlin marble veining)
//
// References:
//   Perlin, "An Image Synthesizer," SIGGRAPH 1985
//   https://iquilezles.org/articles/gradientnoise/
void PerlinNoise3DNode::init()
{
    this->title = "Perlin Noise 3D";

    auto modeProp = this->addEnumProp("output_mode", "Output",
                                      {"Standard", "Ridged", "Billowy",
                                       "Wood", "Marble"});
    modeProp->index = 0;

    this->addIntProp  ("scale",      "Scale",      4,    1,   12,  1);
    this->addFloatProp("z_offset",   "Z Slice",    0.0,  0.0, 10.0, 0.1);
    this->addIntProp  ("octaves",    "Octaves",    6,    1,    8,  1);
    this->addFloatProp("lacunarity", "Lacunarity", 2.0,  1.5,  4.0, 0.1);
    this->addFloatProp("gain",       "Gain",       0.5,  0.2,  0.8, 0.05);
    this->addFloatProp("distortion", "Distortion", 2.0,  0.0,  8.0, 0.1);

    auto source = R""""(
        #define OUT_STANDARD 0
        #define OUT_RIDGED   1
        #define OUT_BILLOWY  2
        #define OUT_WOOD     3
        #define OUT_MARBLE   4

        // Unit-sphere gradient for a 3D cell.
        // XY indices are mod()-wrapped for seamless XY tiling; Z is free.
        vec3 grad3(vec3 cell, float gridSize) {
            vec2 wrapped = mod(cell.xy, vec2(gridSize));
            vec3 p = vec3(wrapped, cell.z)
                   + vec3(_seed * 0.173, _seed * 0.251, _seed * 0.317);
            p = fract(p * vec3(0.1031, 0.1030, 0.0973));
            p += dot(p, p.yzx + 33.33);
            return normalize(fract((p.xxy + p.yxx) * p.zyx) * 2.0 - 1.0);
        }

        // 3D gradient noise with quintic interpolation.
        // Scale factor 1.155 ≈ 1/sqrt(0.75): normalises the theoretical
        // max of sqrt(3/4) for unit gradients in 3D to approximately ±1.
        float gnoise3(vec2 uv, float z, float gridSize) {
            vec3 p = vec3(uv * gridSize, z);
            vec3 i = floor(p);
            vec3 f = fract(p);
            vec3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

            float n000 = dot(grad3(i + vec3(0,0,0), gridSize), f - vec3(0,0,0));
            float n100 = dot(grad3(i + vec3(1,0,0), gridSize), f - vec3(1,0,0));
            float n010 = dot(grad3(i + vec3(0,1,0), gridSize), f - vec3(0,1,0));
            float n110 = dot(grad3(i + vec3(1,1,0), gridSize), f - vec3(1,1,0));
            float n001 = dot(grad3(i + vec3(0,0,1), gridSize), f - vec3(0,0,1));
            float n101 = dot(grad3(i + vec3(1,0,1), gridSize), f - vec3(1,0,1));
            float n011 = dot(grad3(i + vec3(0,1,1), gridSize), f - vec3(0,1,1));
            float n111 = dot(grad3(i + vec3(1,1,1), gridSize), f - vec3(1,1,1));

            return 1.155 * mix(
                mix(mix(n000, n100, u.x), mix(n010, n110, u.x), u.y),
                mix(mix(n001, n101, u.x), mix(n011, n111, u.x), u.y),
                u.z);
        }

        // fBm over gnoise3.  Ridged/Billowy transformations are applied
        // per-octave so the feedback shapes fine detail correctly.
        // Wood/Marble use the raw accumulated noise as turbulence.
        float fbm3(vec2 uv, float z) {
            float value  = 0.0;
            float amp    = 0.5;
            float freq   = float(prop_scale);
            float zScale = 1.0;
            float maxAmp = 0.0;

            for (int i = 0; i < prop_octaves; i++) {
                float gs = max(1.0, floor(freq + 0.5));
                float n  = gnoise3(uv, z * zScale, gs);

                if (prop_output_mode == OUT_RIDGED)
                    n = 1.0 - abs(n);
                else if (prop_output_mode == OUT_BILLOWY)
                    n = abs(n);

                value  += amp * n;
                maxAmp += amp;
                freq   *= prop_lacunarity;
                zScale *= prop_lacunarity;
                amp    *= prop_gain;
            }

            return value / maxAmp;
        }

        vec4 process(vec2 uv) {
            float f = fbm3(uv, prop_z_offset);

            float result;

            if (prop_output_mode == OUT_STANDARD) {
                result = clamp(0.5 + 0.5 * f, 0.0, 1.0);

            } else if (prop_output_mode == OUT_RIDGED ||
                       prop_output_mode == OUT_BILLOWY) {
                result = clamp(f, 0.0, 1.0);

            } else if (prop_output_mode == OUT_WOOD) {
                // Concentric rings centered on UV (0.5, 0.5), distorted by
                // the noise turbulence.  Scale drives ring frequency;
                // Distortion controls how wavy the rings become.
                float rings = length(uv - 0.5) * float(prop_scale) * 2.0
                            + f * prop_distortion;
                result = 0.5 + 0.5 * cos(rings * 6.28318530);

            } else { // MARBLE
                // Horizontal bands distorted by noise turbulence — the
                // canonical Perlin marble.  Scale controls vein frequency;
                // Distortion controls how much the noise bends the veins.
                float vein = uv.x * float(prop_scale)
                           + f * prop_distortion;
                float m = sin(vein * 3.14159265);
                // Gamma curve sharpens vein edges for a more realistic look
                result = clamp(pow(abs(m), 0.6) * sign(m) * 0.5 + 0.5, 0.0, 1.0);
            }

            return vec4(vec3(result), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
