#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"

void ColorNode::init()
{
    this->title = "Color";

    this->addColorProp("color", "Color", QColor(255, 255, 255, 255));

    // todo: add props
    this->setShaderSource(R""""(
        vec4 process(vec2 uv)
        {
            return prop_color;
        }
        )"""");
}

void ColorizeNode::init()
{
    this->title = "Colorize";

    this->addInput("image");

    this->addColorProp("color", "Color", QColor(255, 255, 255, 255));

    // todo: add props
    this->setShaderSource(R""""(
        vec4 process(vec2 uv)
        {
            return texture(image,uv) * prop_color;
        }
        )"""");
}