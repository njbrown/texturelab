#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// Rough Grain — dense field of fine granular dots/specks.
// Produced by layered anisotropic noise with random per-layer orientation and
// per-column phase shifts; the high-frequency phase aliasing breaks the
// would-be streaks into isotropic grains.
// Uses the engine-wide _seed uniform so a single Random Seed change in the
// project regenerates this grain consistently with other procedural nodes.
// Use as: micro-roughness variation on painted surfaces, sandblasted metal
// base, fine textile noise, or the granular substrate of any material.
void RoughGrainNode::init()
{
    this->title = "Rough Grain";

    this->addInput("mask");

    this->addIntProp  ("density",    "Density",     400,   10,   2000,  10);
    this->addFloatProp("grain_size", "Grain Size",  0.04,  0.001, 0.3,  0.001);
    this->addFloatProp("variation",  "Variation",   0.5,   0.0,   1.0,  0.01);
    this->addFloatProp("intensity",  "Intensity",   1.0,   0.0,   2.0,  0.05);
    this->addIntProp  ("layers",     "Layers",      2,     1,     4,    1);

    auto source = R""""(
        #define PI 3.14159265358979

        // 2-D hash with engine-wide seed offset
        float grainHash(vec2 p) {
            p += vec2(_seed * 0.127, _seed * 0.311);
            return hash12(p);
        }

        // One layer of anisotropic noise at a given orientation and frequency.
        // When the perpendicular axis is stretched far more than the primary
        // axis, the per-column random phase aliases the would-be streaks into
        // tightly packed dots — that grain effect is the whole point.
        float grainLayer(vec2 uv, float freq, float angleRad) {
            float cosA = cos(angleRad);
            float sinA = sin(angleRad);

            vec2 rot = vec2(uv.x * cosA + uv.y * sinA,
                           -uv.x * sinA + uv.y * cosA);

            // Heavy anisotropic scaling (drives the dot aliasing)
            rot.x *= freq * 0.05;
            rot.y *= freq * 10.0;

            // Per-column phase perturbation
            float perturb = (grainHash(vec2(rot.x, 0.0)) * 2.0 - 1.0)
                            * prop_variation * 0.5;
            rot.y += perturb * freq;

            float phase = grainHash(vec2(floor(rot.y * float(prop_density)), 0.731));

            float pos   = fract(rot.y * float(prop_density) + phase);
            float halfW = prop_grain_size * 0.5;
            float grain = smoothstep(0.5 - halfW, 0.5, pos)
                        * (1.0 - smoothstep(0.5, 0.5 + halfW, pos));

            return grain;
        }

        vec4 process(vec2 uv)
        {
            float result = 0.0;
            float weight = 1.0;
            float totalW = 0.0;

            for (int i = 0; i < prop_layers; i++) {
                // Random orientation per layer — multi-directional dot field
                float layerAngle = grainHash(vec2(float(i) * 3.7, 1.1)) * PI * 2.0;
                float freq       = 1.0 + float(i) * 0.5;

                result += grainLayer(uv, freq, layerAngle) * weight;
                totalW += weight;
                weight *= 0.55;
            }

            result = (totalW > 0.0) ? result / totalW : 0.0;
            result = clamp(result * prop_intensity, 0.0, 1.0);

            if (mask_connected) {
                result *= texture(mask, uv).r;
            }

            return vec4(vec3(result), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
