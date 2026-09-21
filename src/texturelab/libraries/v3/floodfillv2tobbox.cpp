#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void FloodFillV2ToBBoxNode::init()
{
    this->title = "FF To BBox V2";

    this->addInput("floodfill");

    this->addEnumProp("function", "Function",
                      {"max(x,y)", "min(x,y)", "x", "y", "length(x,y)"});

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 data = texture(floodfill, uv);

            if (data.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);

            // BA = bbox size (width, height) relative to texture
            float intensity = 0.0;

            if (prop_function == 0)
                intensity = max(data.b, data.a);
            else if (prop_function == 1)
                intensity = min(data.b, data.a);
            else if (prop_function == 2)
                intensity = data.b;
            else if (prop_function == 3)
                intensity = data.a;
            else if (prop_function == 4)
                intensity = sqrt(data.b * data.b + data.a * data.a);

            return vec4(vec3(intensity), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
