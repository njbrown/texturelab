#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// https://iquilezles.org/articles/texturerepetition/ (complementary approach)
// Adobe Substance Designer "Make It Tile" reference behaviour.
//
// Offset-cross-blend tiling:
//   1. Sample the input at uv
//   2. Sample the input again at fract(uv + 0.5) — a half-period shift
//   3. Blend toward the shifted copy near the tile edges; keep the original
//      near the centre.
//
// At any tile boundary (uv.x = 0 or 1, uv.y = 0 or 1) the weight is 0 so the
// output reduces to the shifted sample, which evaluates to texture(0.5, *) on
// both sides of the seam — exactly matching across the boundary.
// Tileability is therefore mathematically guaranteed, not approximate.
//
// `Horizontal` and `Vertical` independently toggle which axis is fixed;
// disable either if the input is already tileable in that direction so the
// node leaves it untouched.
void MakeItTileNode::init()
{
    this->title = "Make It Tile";

    this->addInput("image");

    this->addFloatProp("blend_width", "Blend Width", 0.2,  0.02, 0.5, 0.01);
    this->addBoolProp ("horizontal",  "Horizontal",  true);
    this->addBoolProp ("vertical",    "Vertical",    true);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.0, 0.0, 0.0, 1.0);

            // Sample the original
            vec4 orig = texture(image, uv);

            // Build the half-period shift on enabled axes only.
            // Disabled axes use 0 shift, which means orig == shifted along
            // that axis — the blend collapses to a no-op for that direction.
            vec2 shift  = vec2(prop_horizontal ? 0.5 : 0.0,
                               prop_vertical   ? 0.5 : 0.0);
            vec2 sUV    = fract(uv + shift);
            vec4 shifted = texture(image, sUV);

            // Distance to the nearest edge on each enabled axis.
            // For disabled axes use 1.0 so the axis never reduces minD.
            float dx = prop_horizontal ? min(uv.x, 1.0 - uv.x) : 1.0;
            float dy = prop_vertical   ? min(uv.y, 1.0 - uv.y) : 1.0;
            float minD = min(dx, dy);

            // Blend weight: 0 at the seam → use shifted; 1 in centre → use original
            float bw = max(prop_blend_width, 0.001);
            float w  = smoothstep(0.0, bw, minD);

            return mix(shifted, orig, w);
        }
    )"""";

    this->setShaderSource(source);
}
