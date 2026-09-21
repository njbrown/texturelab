#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// https://blog.selfshadow.com/publications/blending-in-detail/
// Barré-Brisebois & Bouchard, "Approximating Translucency for a Fast, Cheap
//   and Convincing Subsurface-Scattering Look," GDC 2011
// Height-aware material blend: the top layer wears away at high-height
// features of the bottom layer, revealing it underneath — physically
// plausible layering without the muddy result of linear alpha blending.
// Classic use: paint coat over scratched metal, where scratches cut through.
// Feed Curvature output as the mask for curvature-driven edge wear.
void HeightBlendNode::init()
{
    this->title = "Height Blend";

    this->addInput("color_a");    // bottom material (e.g. bare metal)
    this->addInput("color_b");    // top material   (e.g. paint)
    this->addInput("height_a");   // height map for bottom material
    this->addInput("height_b");   // height map for top material
    this->addInput("mask");       // wear mask: white=top present, black=bottom exposed

    this->addFloatProp("blend",      "Blend Factor", 0.5, 0.0, 1.0, 0.01);
    this->addFloatProp("edge_width", "Edge Width",   0.1, 0.0, 0.5, 0.01);
    this->addFloatProp("contrast",   "Contrast",     1.0, 0.1, 4.0, 0.1);

    auto source = R""""(
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
    )"""";

    this->setShaderSource(source);
}
