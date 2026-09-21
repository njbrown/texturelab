#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void FloodFillV2ToColorNode::init()
{
    this->title = "FF To Color V2";

    this->addInput("floodfill");
    this->addInput("color");

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 data = texture(floodfill, uv);

            // Background check: bbox size is zero
            if (data.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);

            // Origin is stored directly in RG — no reconstruction needed
            vec2 origin = data.rg;

            return texture(color, origin);
        }
    )"""";

    this->setShaderSource(source);
}
