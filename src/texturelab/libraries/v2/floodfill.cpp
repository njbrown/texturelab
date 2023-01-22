#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

void FloodFillNode::init()
{
    this->title = "Copy";

    this->addInput("image");

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);
            return col;
        }
		)"""";
    this->setShaderSource(source);
}