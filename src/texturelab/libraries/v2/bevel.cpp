#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

BevelNode::init()
{
    this->addInput("image");

    // this->distanceProp =
    this->addFloatProperty("distance", "Distance", 50.0, 0.0, 100.0, 0.01);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);
            return col;
        }
		)"""";
    this->setShaderSource(source);
}