#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void BlendV3Node::init()
{
    this->title = "Blend";

    this->addInput("colorA");   // top / foreground
    this->addInput("colorB");   // bottom / background
    this->addInput("opacity");  // optional mask

    this->addEnumProp("type", "Type", {
        "Normal",
        "Multiply",
        "Screen",
        "Overlay",
        "Soft Light",
        "Hard Light",
        "Color Dodge",
        "Color Burn",
        "Linear Dodge",
        "Linear Burn",
        "Difference",
        "Exclusion",
        "Add",
        "Subtract",
        "Divide",
        "Max",
        "Min",
    });
    this->addFloatProp("opacity", "Opacity", 1.0, 0.0, 1.0, 0.01);

    this->setShaderSource(R""""(
        vec3 blend_screen(vec3 a, vec3 b) {
            return 1.0 - (1.0 - a) * (1.0 - b);
        }

        // Standard overlay: base is bottom layer, blend is top layer.
        // if base < 0.5: 2*base*blend, else 1 - 2*(1-base)*(1-blend)
        vec3 blend_overlay(vec3 base, vec3 blend) {
            return mix(
                2.0 * base * blend,
                1.0 - 2.0 * (1.0 - base) * (1.0 - blend),
                step(vec3(0.5), base)
            );
        }

        // Pegtop / W3C two-case approximation for soft light.
        // base = bottom, blend = top (light source).
        vec3 blend_soft_light(vec3 base, vec3 blend) {
            return mix(
                2.0 * base * blend + base * base * (1.0 - 2.0 * blend),
                2.0 * base * (1.0 - blend) + sqrt(base) * (2.0 * blend - 1.0),
                step(vec3(0.5), blend)
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

            if (prop_type == 0)       // Normal
                result = colA.rgb;
            else if (prop_type == 1)  // Multiply
                result = colA.rgb * colB.rgb;
            else if (prop_type == 2)  // Screen
                result = blend_screen(colA.rgb, colB.rgb);
            else if (prop_type == 3)  // Overlay
                result = blend_overlay(colB.rgb, colA.rgb);
            else if (prop_type == 4)  // Soft Light
                result = blend_soft_light(colB.rgb, colA.rgb);
            else if (prop_type == 5)  // Hard Light
                result = blend_hard_light(colB.rgb, colA.rgb);
            else if (prop_type == 6)  // Color Dodge
                result = blend_color_dodge(colB.rgb, colA.rgb);
            else if (prop_type == 7)  // Color Burn
                result = blend_color_burn(colB.rgb, colA.rgb);
            else if (prop_type == 8)  // Linear Dodge (Add, clamped)
                result = clamp(colA.rgb + colB.rgb, 0.0, 1.0);
            else if (prop_type == 9)  // Linear Burn
                result = clamp(colA.rgb + colB.rgb - 1.0, 0.0, 1.0);
            else if (prop_type == 10) // Difference
                result = abs(colA.rgb - colB.rgb);
            else if (prop_type == 11) // Exclusion
                result = colA.rgb + colB.rgb - 2.0 * colA.rgb * colB.rgb;
            else if (prop_type == 12) // Add (unclamped)
                result = colA.rgb + colB.rgb;
            else if (prop_type == 13) // Subtract
                result = colB.rgb - colA.rgb;
            else if (prop_type == 14) // Divide
                result = clamp(colB.rgb / max(colA.rgb, vec3(1e-4)), 0.0, 1.0);
            else if (prop_type == 15) // Max
                result = max(colA.rgb, colB.rgb);
            else                      // Min
                result = min(colA.rgb, colB.rgb);

            return vec4(mix(colB.rgb, result, finalOpacity), colB.a);
        }
        )"""");
}
