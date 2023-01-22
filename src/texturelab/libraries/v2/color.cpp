#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

void ColorNode::init()
{
    this->title = "Color";

    this->addInput("image");

    // todo: add props
    this->setShaderSource(R""""(
        vec4 process(vec2 uv)
        {
            vec4 prop_color = vec4(1,1,1,1);
            return texture(image,uv) * prop_color;
            //return vec4(uv, vec2(0.0,1.0));// * prop_color;
        }
        )"""");
}