#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// Procedural directional scratch / brushed-metal generator.
// Divides the rotated UV space into N horizontal bands; each band has a
// chance to contain ONE scratch with random start position, length, vertical
// offset within the band, thickness and brightness.
// This discrete-segment approach produces actual visible scratch lines —
// not the aliased dot pattern of frequency-based line noise.
//
// Layer two instances at 0° and ~25° with Screen / Max blend for the classic
// cross-hatched aged metal look used throughout R&C: Rift Apart's weaponry.
void DirectionalScratchesNode::init()
{
    this->title = "Directional Scratches";

    this->addInput("mask");

    this->addFloatProp("angle",      "Angle",            0.0,  -180.0, 180.0, 1.0);
    this->addFloatProp("angle_var",  "Angle Variance",   5.0,    0.0,  45.0,  0.5);
    this->addIntProp  ("count",      "Count",            80,    10,  500,    5);
    this->addFloatProp("density",    "Density",          0.5,    0.0,   1.0,  0.01);
    this->addFloatProp("length",     "Length",           0.6,    0.05,  1.0,  0.01);
    this->addFloatProp("length_var", "Length Variance",  0.5,    0.0,   1.0,  0.05);
    this->addFloatProp("width",      "Width",            0.3,    0.05,  1.0,  0.01);
    this->addFloatProp("waviness",   "Waviness",         0.0,    0.0,   1.0,  0.01);
    this->addFloatProp("intensity",  "Intensity",        1.0,    0.0,   2.0,  0.05);
    this->addIntProp  ("layers",     "Layers",           1,      1,     4,    1);

    auto source = R""""(
        #define PI 3.14159265358979

        // Hash with project-wide seed offset
        float scrHash(vec2 p) {
            return hash12(p + vec2(_seed * 0.131, _seed * 0.379));
        }

        // One layer of discrete scratch line segments.
        // Each band gets at most one scratch with random properties.
        float scratchLayer(vec2 uv, float angleRad, float layerIdx) {
            float cosA = cos(angleRad);
            float sinA = sin(angleRad);

            // Rotate UV into scratch-aligned space; scratches run along +X.
            vec2 rot = vec2(uv.x * cosA + uv.y * sinA,
                           -uv.x * sinA + uv.y * cosA);

            float bandCount = float(prop_count);
            float band      = floor(rot.y * bandCount);
            float bandFract = fract(rot.y * bandCount);

            // Per-band deterministic random properties
            vec2 seed = vec2(band, layerIdx * 113.7);
            float rExist     = scrHash(seed + vec2(0.11, 0.23));
            float rStartX    = scrHash(seed + vec2(0.37, 0.59));
            float rLength    = scrHash(seed + vec2(0.71, 0.97));
            float rOffset    = scrHash(seed + vec2(1.13, 1.31));
            float rThickness = scrHash(seed + vec2(1.51, 1.79));
            float rIntensity = scrHash(seed + vec2(1.97, 2.13));

            // Sparsity: only a fraction of bands actually contain a scratch
            if (rExist > prop_density) return 0.0;

            // Scratch length, with optional variance pulling shorter
            float lenFactor = mix(1.0, rLength, prop_length_var);
            float scratchLen = clamp(prop_length * lenFactor, 0.02, 1.0);

            // X-position along the scratch (wrapped so the pattern tiles in
            // the scratch direction even after rotation)
            float xLocal = fract(rot.x - rStartX);
            if (xLocal > scratchLen) return 0.0;

            // Soft fade at both ends, proportional to length
            float fade = min(0.04, scratchLen * 0.25);
            float xMask = smoothstep(0.0, fade, xLocal)
                        * (1.0 - smoothstep(scratchLen - fade, scratchLen, xLocal));

            // Optional gentle waviness along the scratch (kept within band)
            float wave = sin(xLocal * 28.0 + band * 13.7)
                       * prop_waviness * 0.15;

            // Vertical position within the band (line centre, ±20% jitter)
            float lineCenter = 0.5 + (rOffset - 0.5) * 0.4 + wave;

            // Line profile across band; width prop is fraction of band height
            float dist  = abs(bandFract - lineCenter);
            float halfW = prop_width * 0.25 * mix(0.6, 1.0, rThickness);
            float line  = 1.0 - smoothstep(halfW * 0.5, halfW, dist);

            // Per-scratch brightness variation
            return line * xMask * mix(0.4, 1.0, rIntensity);
        }

        vec4 process(vec2 uv)
        {
            float baseAngle = prop_angle * PI / 180.0;
            float varRad    = prop_angle_var * PI / 180.0;

            float result = 0.0;

            for (int i = 0; i < prop_layers; i++) {
                float fi          = float(i);
                float angleOffset = (scrHash(vec2(fi * 7.31, 0.5)) * 2.0 - 1.0) * varRad;
                float layerAngle  = baseAngle + angleOffset;

                // Take the max (Screen-like) so layers add without dimming
                result = max(result, scratchLayer(uv, layerAngle, fi + 1.0));
            }

            result = clamp(result * prop_intensity, 0.0, 1.0);

            if (mask_connected) {
                result *= texture(mask, uv).r;
            }

            return vec4(vec3(result), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
