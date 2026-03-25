#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void SwirlNode::init()
{
    this->title = "Swirl";
    this->addInput("image");

    this->addFloatProp("radius", "Radius", 0.7, 0.0, 1.0, 0.01);
    this->addFloatProp("angle", "Angle", 90.0, 0.0, 360.0, 0.1);
    this->addFloatProp("centerX", "Center X", 0.5, 0.0, 1.0, 0.01);
    this->addFloatProp("centerY", "Center Y", 0.5, 0.0, 1.0, 0.01);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.0, 0.0, 0.0, 1.0);

            vec2 center = vec2(prop_centerX, prop_centerY);
            vec2 delta = uv - center;
            float len = length(delta);
            float effectAngle = radians(prop_angle);

            float swirlAmount = effectAngle * smoothstep(prop_radius, 0.0, len);
            float a = atan(delta.y, delta.x) + swirlAmount;

            vec2 swirlUV = center + vec2(cos(a), sin(a)) * len;

            return texture(image, swirlUV);
        }
    )"""";

    this->setShaderSource(source);
}
