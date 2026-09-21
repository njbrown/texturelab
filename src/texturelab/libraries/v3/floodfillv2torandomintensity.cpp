#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void FloodFillV2ToRandomIntensityNode::init()
{
    this->title = "FF To Random Intensity V2";

    this->addInput("floodfill");

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 data = texture(floodfill, uv);

            if (data.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);

            vec2 origin = data.rg;

            vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
            color.rgb = vec3(_rand(vec2(_seed) + origin + vec2(1) * vec2(0.01)));

            return color;
        }
    )"""";

    this->setShaderSource(source);
}
