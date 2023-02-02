#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

void GradientDynamicNode::init()
{
    this->title = "Gradient Dynamic";

    this->addInput("image");
    this->addInput("gradient");

    this->addFloatProp("position", "Position", 0, 0, 1, 0.01);

    this->addEnumProp("orientation", "Gradient Direction",
                      {"Vertical", "Horizontal"});

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            float t = 0.0;
    
            float intensity = texture(image, uv).r;

            vec4 col = vec4(0);
            if (prop_orientation == 0)
                col = texture(gradient, vec2(prop_position, intensity));
            else
                col = texture(gradient, vec2(intensity, prop_position));
            
            return col;
        }
        )"""";

    this->setShaderSource(source);
}