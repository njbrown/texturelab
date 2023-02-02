#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

// https://www.mathsisfun.com/polar-cartesian-coordinates.html
void PolarToCartesianNode::init()
{
    this->title = "Polar To Cartesian";

    this->addInput("image");

    this->addFloatProp("angle_offset", "Sample Angle Offset", 0.0, 0, 360, 1);

    auto source = R""""(
        const float PI = 3.1415926538;
        vec4 process(vec2 uv)
        {
            float offset = prop_angle_offset / (360.0) * (PI * 2.0);
            float angle = (uv.x - 0.5) * PI * 2.0 + offset;
            float dist = uv.y * 0.5;

            float x = dist * cos(angle);
            float y = dist * sin(angle);

            vec2 coords = vec2(0.5) + vec2(x, y);

            vec4 col = texture(image, coords);

            return col;
        }
          )"""";

    this->setShaderSource(source);
}