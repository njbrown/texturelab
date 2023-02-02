#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

void HistogramShiftNode::init()
{
    this->title = "Histogram Shift";

    this->addInput("image");

    this->addFloatProp("position", "Position", 0.0, 0.0, 1.0, 0.01);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);

            // shows artifacts for pixels at 1 when position is at 0
            a.rgb = mod(a.rgb + vec3(prop_position), vec3(1.0));

            return a;
        }
        )"""";

    this->setShaderSource(source);
}