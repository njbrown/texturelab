#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// Smith & Bouvier, "Truchet Tilings Revisited," The Mathematical Intelligencer 1987
// https://iquilezles.org/articles/truchet/
// Truchet tile patterns — per-cell randomly oriented arcs or diagonal lines
// that form continuous curved traces across the grid.  Used in sci-fi surface
// tech patterns, circuit board traces and alien architecture detail.
//
// Scale is an integer (cells per UV tile), which combined with the Truchet
// edge-midpoint property guarantees seamless UV tiling: every cell edge
// crosses curves at the same midpoint, so adjacent tiles always connect
// smoothly regardless of per-cell orientation.
void TruchetNode::init()
{
    this->title = "Truchet";

    this->addIntProp  ("scale",      "Scale",      8,   2,   32,   1);
    this->addFloatProp("line_width", "Line Width", 0.08, 0.01,  0.4, 0.01);

    auto variantProp = this->addEnumProp("variant", "Variant",
                                          {"Arc", "Diagonal", "Diagonal Mirrored"});
    variantProp->index = 0;

    auto source = R""""(
        #define VARIANT_ARC      0
        #define VARIANT_DIAG     1
        #define VARIANT_DIAGMIR  2

        // Signed distance from a line segment for anti-aliased lines
        float lineSDF(vec2 p, vec2 a, vec2 b) {
            vec2 ab = b - a;
            vec2 ap = p - a;
            float t = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);
            return length(ap - ab * t);
        }

        vec4 process(vec2 uv)
        {
            vec2 scaled = uv * float(prop_scale);
            vec2 cell   = floor(scaled);
            vec2 local  = fract(scaled) - 0.5; // centred at (0,0) in [-0.5, 0.5]

            // Random orientation per cell (0 or 1), driven by engine seed
            float r = hash12(cell + vec2(_seed * 0.179, _seed * 0.413));
            float orient = step(0.5, r); // 0.0 or 1.0

            float d = 1.0;
            float lw = prop_line_width * 0.5;
            float radius = 0.5;

            if (prop_variant == VARIANT_ARC) {
                // Two quarter-circle arcs connecting midpoints of adjacent edges
                float d1, d2;
                if (orient < 0.5) {
                    d1 = abs(length(local - vec2(-0.5, -0.5)) - radius);
                    d2 = abs(length(local - vec2( 0.5,  0.5)) - radius);
                } else {
                    d1 = abs(length(local - vec2( 0.5, -0.5)) - radius);
                    d2 = abs(length(local - vec2(-0.5,  0.5)) - radius);
                }
                d = min(d1, d2);
            } else if (prop_variant == VARIANT_DIAG) {
                // Midpoint-to-midpoint diagonals (Smith-Truchet variant).
                // Two short segments connect adjacent edge midpoints; every
                // cell touches all 4 edge midpoints regardless of orientation,
                // so adjacent cells always connect smoothly.
                float d1, d2;
                if (orient < 0.5) {
                    // Upper-left and lower-right segments
                    d1 = lineSDF(local, vec2(-0.5, 0.0), vec2(0.0,  0.5));
                    d2 = lineSDF(local, vec2( 0.5, 0.0), vec2(0.0, -0.5));
                } else {
                    // Lower-left and upper-right segments (mirror)
                    d1 = lineSDF(local, vec2(-0.5, 0.0), vec2(0.0, -0.5));
                    d2 = lineSDF(local, vec2( 0.5, 0.0), vec2(0.0,  0.5));
                }
                d = min(d1, d2);
            } else {
                // Two corner-to-corner diagonals forming an X in every cell.
                // The X always touches all 4 corners of the cell, so adjacent
                // cells' diagonals always meet at the shared corner.
                float d1 = lineSDF(local, vec2(-0.5, -0.5), vec2( 0.5,  0.5));
                float d2 = lineSDF(local, vec2( 0.5, -0.5), vec2(-0.5,  0.5));
                d = min(d1, d2);
            }

            // Anti-aliased line with smoothstep
            float aa     = fwidth(d);
            float line   = 1.0 - smoothstep(lw - aa, lw + aa, d);

            return vec4(vec3(line), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
