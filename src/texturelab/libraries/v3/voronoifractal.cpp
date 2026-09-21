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
//
// Seamless tiling: each octave's frequency is snapped to the nearest integer
// grid size, and cell hashes use mod()-wrapped indices.  This makes the
// pattern repeat exactly across UV tile boundaries.  Frequencies are
// effectively rounded — at small Scale values (1-3) you may notice slight
// snapping, but the pattern tiles perfectly at every setting.
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

    auto source = R""""(
        #define DIST_EUCLIDEAN  0
        #define DIST_MANHATTAN  1
        #define DIST_CHEBYSHEV  2
        #define OUT_F1     0
        #define OUT_F2     1
        #define OUT_F2F1   2
        #define OUT_F1F2   3

        // Cell point at index `cellIdx` on a periodic grid of size `gridSize`.
        // The hash uses mod()-wrapped indices so cells at the tile boundary
        // (e.g. cell 0 and cell gridSize) share the same random offset —
        // the prerequisite for seamless tiling.
        vec2 cellPoint(vec2 cellIdx, float gridSize) {
            vec2 wrapped = mod(cellIdx, vec2(gridSize));
            return cellIdx + hash22(wrapped + vec2(_seed * 0.193, _seed * 0.457));
        }

        float cellDist(vec2 a, vec2 b) {
            vec2 d = abs(a - b);
            if (prop_distance == DIST_MANHATTAN)  return d.x + d.y;
            if (prop_distance == DIST_CHEBYSHEV)  return max(d.x, d.y);
            return length(a - b); // Euclidean
        }

        // Returns (F1, F2) for UV sampled on an integer-sized grid.
        vec2 voronoi(vec2 uv, float gridSize) {
            vec2 sampleUV = uv * gridSize;
            vec2 cell     = floor(sampleUV);
            float f1 = 1e9, f2 = 1e9;

            for (int x = -2; x <= 2; x++) {
                for (int y = -2; y <= 2; y++) {
                    vec2 nb = vec2(float(x), float(y));
                    vec2 pt = cellPoint(cell + nb, gridSize);
                    float d = cellDist(sampleUV, pt);
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
                // Snap frequency to integer grid size for seamless tiling
                float gridSize = max(1.0, floor(freq + 0.5));
                vec2 f = voronoi(uv, gridSize);

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
