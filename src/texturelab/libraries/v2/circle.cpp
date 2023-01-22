#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

CircleNode::init()
{
    this->title = "Circle";

    this->addFloatProp("radius", "Radius", 0.2, 0, 1, 0.01);
    this->addEnumProp("color_gen", "Color Generation",
                      {"Flat", "Linear", "Exponential"});

    this->setShaderSource(R""""(
        vec4 process(vec2 uv)
        {
            float dist = distance(uv, vec2(0.5));
            if( dist <= prop_radius) {
                if (prop_color_gen==0)
                    return vec4(vec3(1.0), 1.0);
                else if (prop_color_gen==1)
                    return vec4(vec3(1.0 - dist / prop_radius), 1.0);
                else if (prop_color_gen==2)
                {
                    float val = dist / prop_radius;
                    return vec4(vec3(1.0 - val * val), 1.0);
                }
                
            }

            return vec4(vec3(0.0), 1.0);
        }
        )"""");
}