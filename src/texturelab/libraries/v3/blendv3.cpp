#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void BlendV3Node::init()
{
    this->title = "Blend";

    this->addInput("colorA");  // top / foreground
    this->addInput("colorB");  // bottom / background
    this->addInput("opacity"); // optional mask

    // Indices 0-8 match the v1 blend node for consistency.
    // New modes are appended from index 9 onward.
    this->addEnumProp("type", "Type",
                      {
                          "Multiply",     // 0
                          "Add",          // 1
                          "Subtract",     // 2
                          "Divide",       // 3
                          "Max",          // 4
                          "Min",          // 5
                          "Switch",       // 6  (shows colorA — same as v1)
                          "Overlay",      // 7  (fixed: correct 2× factor)
                          "Screen",       // 8
                          "Soft Light",   // 9
                          "Hard Light",   // 10
                          "Color Dodge",  // 11
                          "Color Burn",   // 12
                          "Linear Dodge", // 13
                          "Linear Burn",  // 14
                          "Difference",   // 15
                          "Exclusion",    // 16
                          "Normal",       // 17
                      });
    this->addFloatProp("opacity", "Opacity", 1.0, 0.0, 1.0, 0.01);

    this->setShaderSource(R""""(
        vec3 blend_screen(vec3 a, vec3 b) {
            return 1.0 - (1.0 - a) * (1.0 - b);
        }

        // Standard overlay: base is bottom layer, blend is top layer.
        // if base < 0.5: 2*base*blend, else 1 - 2*(1-base)*(1-blend)
        // Uses explicit per-channel conditionals to avoid evaluating both
        // branches simultaneously, which can produce NaN/Inf with HDR inputs.
        vec3 blend_overlay(vec3 base, vec3 blend) {
            vec3 dark  = 2.0 * base * blend;
            vec3 light = 1.0 - 2.0 * (1.0 - base) * (1.0 - blend);
            return vec3(
                base.r < 0.5 ? dark.r : light.r,
                base.g < 0.5 ? dark.g : light.g,
                base.b < 0.5 ? dark.b : light.b
            );
        }

        // Pegtop / W3C two-case approximation for soft light.
        // base = bottom, blend = top (light source).
        vec3 blend_soft_light(vec3 base, vec3 blend) {
            vec3 dark  = 2.0 * base * blend + base * base * (1.0 - 2.0 * blend);
            vec3 light = 2.0 * base * (1.0 - blend) + sqrt(clamp(base, 0.0, 1.0)) * (2.0 * blend - 1.0);
            return vec3(
                blend.r < 0.5 ? dark.r : light.r,
                blend.g < 0.5 ? dark.g : light.g,
                blend.b < 0.5 ? dark.b : light.b
            );
        }

        // Hard light = overlay with layers swapped.
        vec3 blend_hard_light(vec3 base, vec3 blend) {
            return blend_overlay(blend, base);
        }

        vec3 blend_color_dodge(vec3 base, vec3 blend) {
            return clamp(base / max(1.0 - blend, vec3(1e-4)), 0.0, 1.0);
        }

        vec3 blend_color_burn(vec3 base, vec3 blend) {
            return clamp(1.0 - (1.0 - base) / max(blend, vec3(1e-4)), 0.0, 1.0);
        }

        vec4 process(vec2 uv)
        {
            float finalOpacity = prop_opacity;
            if (opacity_connected)
                finalOpacity *= texture(opacity, uv).r;

            vec4 colA = texture(colorA, uv); // top / foreground
            vec4 colB = texture(colorB, uv); // bottom / background
            vec3 result = colB.rgb;

            if (prop_type == 0)       // Multiply
                result = colA.rgb * colB.rgb;
            else if (prop_type == 1)  // Add
                result = colA.rgb + colB.rgb;
            else if (prop_type == 2)  // Subtract
                result = colB.rgb - colA.rgb;
            else if (prop_type == 3)  // Divide
                result = colB.rgb / max(colA.rgb, vec3(1e-4));
            else if (prop_type == 4)  // Max
                result = max(colA.rgb, colB.rgb);
            else if (prop_type == 5)  // Min
                result = min(colA.rgb, colB.rgb);
            else if (prop_type == 6)  // Switch (show colorA)
                result = colA.rgb;
            else if (prop_type == 7)  // Overlay (fixed)
                result = blend_overlay(colB.rgb, colA.rgb);
            else if (prop_type == 8)  // Screen
                result = blend_screen(colA.rgb, colB.rgb);
            else if (prop_type == 9)  // Soft Light
                result = blend_soft_light(colB.rgb, colA.rgb);
            else if (prop_type == 10) // Hard Light
                result = blend_hard_light(colB.rgb, colA.rgb);
            else if (prop_type == 11) // Color Dodge
                result = blend_color_dodge(colB.rgb, colA.rgb);
            else if (prop_type == 12) // Color Burn
                result = blend_color_burn(colB.rgb, colA.rgb);
            else if (prop_type == 13) // Linear Dodge (clamped Add)
                result = clamp(colA.rgb + colB.rgb, 0.0, 1.0);
            else if (prop_type == 14) // Linear Burn
                result = clamp(colA.rgb + colB.rgb - 1.0, 0.0, 1.0);
            else if (prop_type == 15) // Difference
                result = abs(colA.rgb - colB.rgb);
            else if (prop_type == 16) // Exclusion
                result = colA.rgb + colB.rgb - 2.0 * colA.rgb * colB.rgb;
            else                      // Normal (17)
                result = colA.rgb;

            return vec4(mix(colB.rgb, result, finalOpacity), colB.a);
        }
        )"""");
}
