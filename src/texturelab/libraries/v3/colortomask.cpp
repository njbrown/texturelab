#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// https://en.wikipedia.org/wiki/HSL_and_HSV
// Converts a target colour range in an input image to a greyscale mask.
// Uses HSL-space distance for perceptually accurate colour matching — far
// more reliable than RGB Euclidean distance.
// Primary use: extract zones from a flat-colour ID map to drive per-material
// Height Blend, roughness or metalness branches.
void ColorToMaskNode::init()
{
    this->title = "Color To Mask";

    this->addInput("image");

    this->addColorProp("target_color", "Target Color", QColor(255, 0, 0));
    this->addFloatProp("hue_range",    "Hue Range",    1.0,  0.1, 4.0, 0.1);
    this->addFloatProp("sat_range",    "Sat Range",    0.5,  0.0, 4.0, 0.1);
    this->addFloatProp("lum_range",    "Lum Range",    0.3,  0.0, 4.0, 0.1);
    this->addFloatProp("softness",     "Softness",     0.2,  0.01, 1.0, 0.01);
    this->addBoolProp ("invert",       "Invert",       false);

    auto source = R""""(
        vec3 rgb2hsl(vec3 c) {
            float maxC  = max(c.r, max(c.g, c.b));
            float minC  = min(c.r, min(c.g, c.b));
            float delta = maxC - minC;

            float h = 0.0;
            if (delta > 0.001) {
                if (maxC == c.r)      h = mod((c.g - c.b) / delta, 6.0);
                else if (maxC == c.g) h = (c.b - c.r) / delta + 2.0;
                else                  h = (c.r - c.g) / delta + 4.0;
                h /= 6.0;
                if (h < 0.0) h += 1.0;
            }
            float l = (maxC + minC) * 0.5;
            float s = (delta < 0.001) ? 0.0
                      : delta / (1.0 - abs(2.0 * l - 1.0));
            return vec3(h, s, l);
        }

        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.0, 0.0, 0.0, 1.0);

            vec3 col       = texture(image, uv).rgb;
            vec3 hsl       = rgb2hsl(col);
            vec3 targetHSL = rgb2hsl(prop_target_color.rgb);

            // Hue distance is circular
            float hueDist = abs(hsl.x - targetHSL.x);
            hueDist = min(hueDist, 1.0 - hueDist);

            float dist = hueDist              * prop_hue_range
                       + abs(hsl.y - targetHSL.y) * prop_sat_range
                       + abs(hsl.z - targetHSL.z) * prop_lum_range;

            float mask = 1.0 - smoothstep(0.0, prop_softness, dist);

            if (prop_invert) mask = 1.0 - mask;

            return vec4(vec3(clamp(mask, 0.0, 1.0)), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
