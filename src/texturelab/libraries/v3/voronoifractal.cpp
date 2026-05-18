#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// Worley, "A Cellular Texture Basis Function," SIGGRAPH 1996
// https://iquilezles.org/articles/voronoilines/
// Multi-octave Worley noise (fractal Voronoi).  Each octave doubles the
// frequency and halves the amplitude (Lacunarity / Gain).
// Output modes: F1 = distance to nearest cell (soft blobs),
//               F2 = second nearest, F2-F1 = cell edges only (crack lines),
//               F1+F2 = softer combined cells.
void VoronoiFractalNode::init()
{
    this->title = "Voronoi Fractal";

    this->addFloatProp("scale",      "Scale",         4.0,  0.5, 16.0, 0.5);
    this->addIntProp  ("octaves",    "Octaves",        4,    1,    8,   1);
    this->addFloatProp("lacunarity", "Lacunarity",     2.0,  1.5,  4.0, 0.1);
    this->addFloatProp("gain",       "Gain",           0.5,  0.2,  0.8, 0.05);

    auto distProp = this->addEnumProp("distance", "Distance Func",
                                      {"Euclidean", "Manhattan", "Chebyshev"});
    distProp->index = 0;

    auto outProp = this->addEnumProp("output_mode", "Output",
                                     {"F1", "F2", "F2-F1", "F1+F2"});
    outProp->index = 0;

    this->addIntProp("seed", "Seed", 0, 0, 999, 1);

    auto source = R""""(
        #define DIST_EUCLIDEAN  0
        #define DIST_MANHATTAN  1
        #define DIST_CHEBYSHEV  2
        #define OUT_F1     0
        #define OUT_F2     1
        #define OUT_F2F1   2
        #define OUT_F1F2   3

        vec2 cellPoint(vec2 cell) {
            return cell + hash22(cell + vec2(float(prop_seed) * 0.193, float(prop_seed) * 0.457));
        }

        float cellDist(vec2 a, vec2 b) {
            vec2 d = abs(a - b);
            if (prop_distance == DIST_MANHATTAN)  return d.x + d.y;
            if (prop_distance == DIST_CHEBYSHEV)  return max(d.x, d.y);
            return length(a - b); // Euclidean
        }

        // Returns (F1, F2) for a given scaled UV
        vec2 voronoi(vec2 uv) {
            vec2 cell  = floor(uv);
            vec2 local = fract(uv);
            float f1 = 1e9, f2 = 1e9;

            for (int x = -2; x <= 2; x++) {
                for (int y = -2; y <= 2; y++) {
                    vec2 nb = vec2(float(x), float(y));
                    vec2 pt = cellPoint(cell + nb);
                    float d = cellDist(uv, pt);
                    if (d < f1) { f2 = f1; f1 = d; }
                    else if (d < f2) { f2 = d; }
                }
            }
            // Normalise: in Euclidean space max F1 for uniform random points ≈ 0.67
            return vec2(f1, f2) / 0.67;
        }

        vec4 process(vec2 uv)
        {
            float value     = 0.0;
            float amplitude = 1.0;
            float freq      = prop_scale;
            float totalAmp  = 0.0;

            for (int i = 0; i < prop_octaves; i++) {
                vec2 f = voronoi(uv * freq);

                float octaveVal = 0.0;
                if (prop_output_mode == OUT_F1)
                    octaveVal = f.x;
                else if (prop_output_mode == OUT_F2)
                    octaveVal = f.y;
                else if (prop_output_mode == OUT_F2F1)
                    octaveVal = clamp(f.y - f.x, 0.0, 1.0);
                else
                    octaveVal = clamp((f.x + f.y) * 0.5, 0.0, 1.0);

                value    += octaveVal * amplitude;
                totalAmp += amplitude;
                freq     *= prop_lacunarity;
                amplitude *= prop_gain;
            }

            float result = clamp(value / totalAmp, 0.0, 1.0);
            return vec4(vec3(result), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
