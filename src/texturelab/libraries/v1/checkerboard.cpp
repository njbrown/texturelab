#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"

#include <QColor>

void CheckerboardNode::init()
{
    this->title = "Checkerboard";

    this->addFloatProp("rows", "Rows", 2, 1, 20, 1);
    this->addFloatProp("columns", "Columns", 2, 1, 20, 1);

    this->addColorProp("color", "Color", QColor());

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            if ((mod(prop_columns*uv.x, 1.0) < 0.5) ^^ (mod(prop_rows*uv.y, 1.0) < 0.5))
            {
                return vec4(prop_color.rgb, 1.0);
            }
            else
            {
                return vec4(1.0, 1.0, 1.0, 1.0);
            }
        }
        )"""";

    this->setShaderSource(source);
}