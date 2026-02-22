#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"

void CopyNode::init()
{
    this->title = "Copy";

    this->addInput("image");
    this->addStringProp("name", "Name");

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);
            return col;
        }
        )"""";

    this->setShaderSource(source);
}