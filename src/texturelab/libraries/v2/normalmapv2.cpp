#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

// http://www.smart-page.net/smartnormal/js/SmartNormalMapFilter.js
void NormalMapV2Node::init()
{
    this->title = "Normal Map";
    // this.exportName = "result";
    this->addInput("height");

    // this->addFloatProp("strength", "Strength", 0.02, -0.02, 0.02,
    // 0.00001);
    this->addFloatProp("strength", "Strength", 1, -2.0, 2.0, 0.05);
    this->addIntProp("range", "Range", 1, 1, 20, 1);
    this->addBoolProp("res_ind", "Resolution Independent", false);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec2 step = (vec2(1.0, 1.0) / _textureSize) * float(prop_range);
            if (prop_res_ind)
                step = (vec2(1.0, 1.0) / 1024.0) * float(prop_range);
            float strength = prop_strength * 0.1;

            // center point
            float d0 = abs(texture(height, uv + vec2(0.0, 0.0)).r) * strength / 2.0;

            // sample horizontally
            float d1 = abs(texture(height, uv + vec2(step.x, 0.0)).r) * strength / 2.0;
            float d2 = abs(texture(height, uv + vec2(-step.x, 0.0)).r) * strength / 2.0;

            // sample vertically
            float d3 = abs(texture(height, uv + vec2(0.0, step.y)).r) * strength / 2.0;
            float d4 = abs(texture(height, uv + vec2(0.0, -step.y)).r) * strength / 2.0;

            // find diff horizontally and average
            float dx = ((d2 - d0) + (d0 - d1)) * 0.5;

            // find diff vertically and average
            float dy = ((d4 - d0) + (d0 - d3)) * 0.5;

            vec3 dvx = vec3(step.x, 0.0       , d1-d0);
            vec3 dvy = vec3(0.0       , step.y, d3-d0);
            vec3 normal = normalize(cross(dvx, dvy));
            vec3 final = normal.xyz * 0.5 + 0.5;

            return vec4(final, 1.0);
        }
        )"""";

    this->setShaderSource(source);
}