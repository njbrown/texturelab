#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"

void ThresholdNode::init()
{
    this->title = "Threshold";

    this->addInput("image");

    this->addFloatProp("threshold", "Threshold", 0.0, 0.0, 1.0, 0.01);
    this->addBoolProp("invert", "Invert", true);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);

            if (prop_invert)
                a.rgb = step(1.0 - prop_threshold, a.rgb);
            else
                a.rgb = step(prop_threshold, a.rgb);

            return a;
        }
        )"""";

    this->setShaderSource(source);
}