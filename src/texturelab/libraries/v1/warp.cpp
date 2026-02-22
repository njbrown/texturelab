#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"

void WarpNode::init()
{
    this->title = "Warp";

    this->addInput("inputImage");
    this->addInput("height");

    this->addFloatProp("intensity", "Intensity", 0.1, -1.0, 1.0, 0.01);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec2 step = vec2(1.0,1.0)/_textureSize;
            vec4 warpCol = texture(height, uv);
            float warp = (warpCol.r + warpCol.g + warpCol.b) / 3.0;

            vec4 color = texture(inputImage, uv + (vec2(warp) - 0.5) * vec2(1.0, -1.0) * prop_intensity);

            return color;
        }
        )"""";

    this->setShaderSource(source);
}