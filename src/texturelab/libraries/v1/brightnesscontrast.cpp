#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"

void BrightnessContrastNode::init()
{
    this->title = "Brightness Contrast";

    this->addInput("image");

    this->addFloatProp("contrast", "Contrast", 0.0, -1, 1, 0.1);
    this->addFloatProp("brightness", "Brightness", 0.0, -1, 1, 0.1);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);

            col.rgb += prop_brightness;
            if (prop_contrast > 0.0)
                col.rgb = (col.rgb - 0.5) / (1.0 - prop_contrast) + 0.5;
            else
                col.rgb = (col.rgb - 0.5) * (1.0 + prop_contrast) + 0.5;

            return col;
        }
        )"""";

    this->setShaderSource(source);
}