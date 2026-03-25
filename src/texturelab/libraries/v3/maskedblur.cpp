#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void MaskedBlurNode::init()
{
    this->title = "Masked Blur";
    this->addInput("image");
    this->addInput("mask");

    this->addFloatProp("intensity", "Intensity", 1.0, 0.0, 10.0, 0.1);
    this->addIntProp("samples", "Samples", 50, 1, 100, 1);
    this->addFloatProp("opacity", "Opacity", 0.5, 0.0, 1.0, 0.01);

    auto source = R""""(
        #define PI 3.14159265359
        #define POW2(x) ((x) * (x))

        float gaussian(vec2 i, float sigma)
        {
            return 1.0 / (2.0 * PI * POW2(sigma))
                 * exp(-(POW2(i.x) + POW2(i.y)) / (2.0 * POW2(sigma)));
        }

        vec3 blur(sampler2D sp, vec2 uv, vec2 scale)
        {
            vec3 col = vec3(0.0);
            float accum = 0.0;
            float sigma = float(prop_samples) * 0.25;
            int half_samples = prop_samples / 2;

            for (int x = -half_samples; x < half_samples; x++)
            for (int y = -half_samples; y < half_samples; y++)
            {
                vec2 offset = vec2(float(x), float(y));
                float weight = gaussian(offset, sigma);
                col += texture(sp, uv + scale * offset).rgb * weight;
                accum += weight;
            }

            return col / accum;
        }

        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.0, 0.0, 0.0, 1.0);

            float maskVal = mask_connected ? 1.0 - texture(mask, uv).r : 1.0;
            float blurAmount = 1.0 - maskVal * prop_opacity;

            vec2 ps = vec2(1.0) / _textureSize;
            vec4 color;
            color.rgb = blur(image, uv, ps * prop_intensity * blurAmount);
            color.a = 1.0;

            return color;
        }
    )"""";

    this->setShaderSource(source);
}
