#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// Packs four textures into a 2x2 atlas. Each input is scaled down to fill
// its own quadrant. Unconnected quadrants show the background color.
void Atlas2x2Node::init()
{
    this->title = "Atlas 2x2";

    this->addInput("top_left");
    this->addInput("top_right");
    this->addInput("bottom_left");
    this->addInput("bottom_right");

    this->addColorProp("background", "Background Color", QColor(0, 0, 0));

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            // uv.y == 1.0 is the top of the image
            vec2 cell = step(vec2(0.5), uv);
            vec2 localUV = fract(uv * 2.0);

            if (cell.y > 0.5) {
                if (cell.x < 0.5)
                    return top_left_connected ? texture(top_left, localUV) : prop_background;
                return top_right_connected ? texture(top_right, localUV) : prop_background;
            }

            if (cell.x < 0.5)
                return bottom_left_connected ? texture(bottom_left, localUV) : prop_background;
            return bottom_right_connected ? texture(bottom_right, localUV) : prop_background;
        }
    )"""";

    this->setShaderSource(source);
}
