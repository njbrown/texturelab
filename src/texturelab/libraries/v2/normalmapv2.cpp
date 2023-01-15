#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

// http://www.smart-page.net/smartnormal/js/SmartNormalMapFilter.js
NormalMapNodeV2::init()
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

BetterWarpNode::init()
{
    this->title = "Better Warp Node";
    // this.exportName = "result";
    this->addInput("image");
    this->addInput("height");

    this->addFloatProp("strength", "Strength", 0.001, -0.02, 0.02, 0.00001);
    this->addFloatProp("spread", "Spread", 0.1, 0, 1, 0.01);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            //vec2 size = textureSize(height);
            //vec2 size = vec2(1024,1024); // quick hack for now
            vec2 step = vec2(1.0,1.0)/_textureSize;

            // center point
            float d0 = abs(texture(height, uv + vec2(0.0, 0.0)).r) * prop_strength / 2.0;

            // sample horizontally
            float d1 = abs(texture(height, uv + vec2(step.x, 0.0)).r) * prop_strength / 2.0;
            float d2 = abs(texture(height, uv + vec2(-step.x, 0.0)).r) * prop_strength / 2.0;

            // sample vertically
            float d3 = abs(texture(height, uv + vec2(0.0, step.y)).r) * prop_strength / 2.0;
            float d4 = abs(texture(height, uv + vec2(0.0, -step.y)).r) * prop_strength / 2.0;

            // find diff horizontally and average
            float dx = ((d2 - d0) + (d0 - d1)) * 0.5;

            // find diff vertically and average
            float dy = ((d4 - d0) + (d0 - d3)) * 0.5;

            // calculate normal
            //vec3 normal = normalize(vec3(dx * prop_strength, dy * prop_strength, 1.0));
            //vec3 normal = normalize(vec3(dx, dy, 1.0));
            //vec3 final = normal.xyz * 0.5 + 0.5; // bring to 0.0 - 1.0 range

            vec3 dvx = vec3(step.x, 0.0       , d1-d0);
            vec3 dvy = vec3(0.0       , step.y, d3-d0);
            vec3 normal = normalize(cross(dvx, dvy));
            //vec3 final = normal.xyz * 0.5 + 0.5;

            float intensity = (1.0 - texture(height, uv).r) * prop_spread;
            vec3 colorOut = texture(image, uv + vec2(-normal.x * intensity, -normal.y * intensity) ).rgb;

            return vec4(colorOut, 1.0);
            //return vec4(vec3(normal.y), 1.0);
            //return vec4(vec3(dot(normal, vec3(0,0,1))), 1.0);
        }
        )"""";

    this->setShaderSource(source);
}
