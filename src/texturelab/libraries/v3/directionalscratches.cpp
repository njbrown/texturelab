#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// Procedural directional scratch / brushed-metal generator.
// Divides the rotated UV space into N horizontal bands; each band has a
// chance to contain ONE scratch with random start position, length, vertical
// offset, thickness and brightness.
//
// Seamless tiling: angle is restricted to a stepped enum of geometrically
// tileable orientations (0°, 45°, 90°, 135°).  Each option uses an
// integer-coefficient rotation matrix so that a UV-tile shift translates to
// an integer band-index shift; combined with mod()-wrapped band indices and
// fract()-wrapped scratch coordinates, the pattern tiles perfectly.
void DirectionalScratchesNode::init()
{
    this->title = "Directional Scratches";

    auto angleProp = this->addEnumProp("angle", "Angle",
        {"0° (Horizontal)", "45°", "90° (Vertical)", "135°"});
    angleProp->index = 0;

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

        #define ANGLE_0    0
        #define ANGLE_45   1
        #define ANGLE_90   2
        #define ANGLE_135  3

        // Hash with project-wide seed offset
        float scrHash(vec2 p) {
            return hash12(p + vec2(_seed * 0.131, _seed * 0.379));
        }

        // Integer-coefficient rotation: preserves the property that a UV
        // shift of (1,0) or (0,1) produces an integer rotated-coordinate
        // shift, which is what makes the pattern tile perfectly.
        // Diagonal angles (45°/135°) come out scaled by √2 — this just means
        // diagonal patterns appear √2 denser per UV tile than cardinal ones.
        vec2 rotateUV(vec2 uv) {
            if (prop_angle == ANGLE_0)   return uv;
            if (prop_angle == ANGLE_45)  return vec2(uv.x + uv.y, uv.y - uv.x);
            if (prop_angle == ANGLE_90)  return vec2(uv.y, -uv.x);
            /* ANGLE_135 */              return vec2(uv.y - uv.x, -uv.x - uv.y);
        }

        // One layer of discrete scratch line segments.
        // Each band gets at most one scratch with random properties.
        float scratchLayer(vec2 uv, float layerIdx) {
            vec2 rot = rotateUV(uv);

            float bandCount = float(prop_count);

            // mod() wraps band index to [0, bandCount-1] so band hashes match
            // across UV-tile boundaries.  Integer-rotation guarantees that
            // floor(rot.y * bandCount) shifts by exactly bandCount across one
            // UV tile, so the mod is exact.
            float band      = mod(floor(rot.y * bandCount), bandCount);
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
            float lenFactor  = mix(1.0, rLength, prop_length_var);
            float scratchLen = clamp(prop_length * lenFactor, 0.02, 1.0);

            // X-position along the scratch — fract handles tiling automatically
            float xLocal = fract(rot.x - rStartX);
            if (xLocal > scratchLen) return 0.0;

            // Soft fade at both ends, proportional to length
            float fade  = min(0.04, scratchLen * 0.25);
            float xMask = smoothstep(0.0, fade, xLocal)
                        * (1.0 - smoothstep(scratchLen - fade, scratchLen, xLocal));

            // Optional gentle waviness along the scratch
            float wave = sin(xLocal * 28.0 + band * 13.7)
                       * prop_waviness * 0.15;

            // Vertical position within the band
            float lineCenter = 0.5 + (rOffset - 0.5) * 0.3 + wave;

            // Line profile across band; width is fraction of band height
            float dist  = abs(bandFract - lineCenter);
            float halfW = prop_width * 0.25 * mix(0.6, 1.0, rThickness);
            float line  = 1.0 - smoothstep(halfW * 0.5, halfW, dist);

            // Per-scratch brightness variation
            return line * xMask * mix(0.4, 1.0, rIntensity);
        }

        vec4 process(vec2 uv)
        {
            float result = 0.0;
            for (int i = 0; i < prop_layers; i++) {
                // Each layer generates an independent scratch set via the hash
                result = max(result, scratchLayer(uv, float(i) + 1.0));
            }
            return vec4(vec3(clamp(result * prop_intensity, 0.0, 1.0)), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
