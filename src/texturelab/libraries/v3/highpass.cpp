#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// https://en.wikipedia.org/wiki/Unsharp_masking
// Highpass = input - blur(input) + 0.5
// Output is neutral-gray where there is no detail; overlay-blend it onto a
// base material to layer micro-variation without altering base tone.
void HighpassNode::init()
{
    this->title = "Highpass";

    this->addInput("image");

    this->addFloatProp("radius",    "Radius",    8.0,  0.5, 64.0, 0.5);
    this->addIntProp  ("quality",   "Quality",   4,    1,   8,    1);
    this->addFloatProp("intensity", "Intensity", 1.0,  0.0, 4.0,  0.1);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.5, 0.5, 0.5, 1.0);

            vec4 original = texture(image, uv);

            // Box blur approximation of Gaussian
            vec4 blurred = vec4(0.0);
            float total  = 0.0;
            int   q      = prop_quality;

            for (int x = -q; x <= q; x++) {
                for (int y = -q; y <= q; y++) {
                    vec2 offset = vec2(float(x), float(y))
                                  * prop_radius / _textureSize.xy;
                    blurred += texture(image, uv + offset);
                    total   += 1.0;
                }
            }
            blurred /= total;

            // Highpass centered on 0.5 so it is neutral for Overlay blending
            vec4 detail = (original - blurred) * prop_intensity + 0.5;

            return clamp(detail, 0.0, 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
