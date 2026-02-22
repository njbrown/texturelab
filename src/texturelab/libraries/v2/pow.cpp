#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

void PowNode::init()
{
    this->title = "Pow";

    this->addInput("image");

    this->addFloatProp("exponent", "Exponent", 1.0, 0.0, 10.0, 0.01);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);
            a.rgb = pow(a.rgb, vec3(prop_exponent));

            return a;
        }
        )"""";

    this->setShaderSource(source);
}
