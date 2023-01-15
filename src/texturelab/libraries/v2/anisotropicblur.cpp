#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

// uses single pass gaussian
// https://www.shadertoy.com/view/4tSyzy
// https://stackoverflow.com/questions/2157920/why-define-pi-4atan1-d0
AnisotropicBlurNode::init()
{
    this->title = "Anisotropic Blur";

    this->addInput("image");

    this->addFloatProp("intensity", "Intensity", 2, 0, 10, 0.1);
    this->addFloatProp("anisotropy", "Anisotropy", 0.7, 0, 1.0, 0.1);
    this->addIntProp("samples", "Samples", 50, 0, 100, 1);
    this->addFloatProp("angle", "Angle", 0, 0, 360, 1);

    auto source = R""""(
        #define pow2(x) (x * x)

        const float pi = atan(1.0) * 4.0;

        float gaussian(vec2 i, float sigma) {
            return 1.0 / (2.0 * pi * pow2(sigma)) * exp(-((pow2(i.x) + pow2(i.y)) / (2.0 * pow2(sigma))));
        }

        mat2 buildRot(float rot)
        {
            float r = radians(rot);
            return mat2(cos(r), -sin(r), sin(r), cos(r));
        }

        vec3 blur(sampler2D sp, vec2 uv, vec2 scale) {
            vec3 col = vec3(0.0);
            float accum = 0.0;
            float weight;
            vec2 offset;

            float sigma = float(prop_samples) * 0.25;
            float aniso = 1.0 - prop_anisotropy;
            
            for (int x = -prop_samples / 2; x < prop_samples / 2; ++x) {
                for (int y = -prop_samples / 2; y < prop_samples / 2; ++y) {
                    offset = vec2(x, y);
                    weight = gaussian(offset, sigma);
                    
                    offset.y *= aniso;
                    offset = buildRot(prop_angle) * offset;

                    col += texture(sp, uv + scale * offset).rgb * weight;
                    accum += weight;
                }
            }
            
            return col / accum;
        }

        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0,0,0,1.0);

            vec4 color = vec4(0.0);
            vec2 ps = vec2(1.0, 1.0) / _textureSize;
            color.rgb = blur(image, uv, ps * prop_intensity);
            color.a = 1.0;

            return color;
        }
        )"""";

    this.buildShader(source);
}
