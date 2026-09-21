#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"

void HeightShiftNode::init()
{
    this->title = "Height Shift";

    this->addInput("image");

    this->addFloatProp("shift", "Shift", 0.0, -1.0, 1.0, 0.01);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);

            return a + vec4(vec3(prop_shift), 0.0);
        }
        )"""";

    this->setShaderSource(source);
}