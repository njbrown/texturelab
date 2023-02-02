#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

void QuantizeNode::init()
{
    this->title = "Quantize";

    this->addInput("image");

    this->addIntProp("steps", "Steps", 12, 2, 20, 1);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);
            a.rgb = floor(a.rgb * vec3(float(prop_steps))) / vec3(float(prop_steps));

            return a;
        }
        )"""";

    this->setShaderSource(source);
}
