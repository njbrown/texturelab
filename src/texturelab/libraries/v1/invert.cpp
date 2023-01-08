#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"

void InvertNode::init()
{
    this->title = "Invert";

    this->addInput("color");

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 col = vec4(1.0) - texture(color,uv);
            col.a = 1.0;
            return col;
        }
        )"""";

    this->setShaderSource(source);
}