#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// Quick utility to attach an alpha channel to a texture: RGB comes from
// `rgba`, alpha comes from a chosen channel of `alpha`. Saves building a
// 4-input RGBA Merge graph just to swap one channel.
void SetAlphaNode::init()
{
    this->title = "Set Alpha";

    this->addInput("rgba");
    this->addInput("alpha");

    auto prop = this->addEnumProp(
        "alphaChannel", "Alpha Channel",
        {"Red", "Green", "Blue", "Alpha", "Average (RGB)"});
    prop->setValue(0);

    this->addBoolProp("invert", "Invert Alpha", false);

    auto source = R""""(
        float getChannel(vec4 inputData, int mode)
        {
            if (mode == 0) return inputData.r;
            if (mode == 1) return inputData.g;
            if (mode == 2) return inputData.b;
            if (mode == 3) return inputData.a;
            if (mode == 4) {
                return (inputData.r + inputData.g + inputData.b) * 0.3333333;
            }

            return 0.0;
        }

        vec4 process(vec2 uv)
        {
            vec3 rgb = rgba_connected ? texture(rgba, uv).rgb : vec3(0.0);

            float a = 1.0;
            if (alpha_connected) {
                a = getChannel(texture(alpha, uv), prop_alphaChannel);
                if (prop_invert) a = 1.0 - a;
            }

            return vec4(rgb, a);
        }
    )"""";

    this->setShaderSource(source);
}
