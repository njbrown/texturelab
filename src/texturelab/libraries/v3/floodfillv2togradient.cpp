#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void FloodFillV2ToGradientNode::init()
{
    this->title = "FF To Gradient V2";

    this->addInput("floodfill");

    this->addFloatProp("angle", "Angle", 0, 0, 360, 1);
    this->addFloatProp("variation", "Angle Variation", 0, 0, 1.0, 0.05);

    auto source = R""""(
        mat2 buildRot(float rot)
        {
            float r = radians(rot);
            return mat2(cos(r), -sin(r), sin(r), cos(r));
        }

        float distAlongDir(vec2 x, vec2 dir)
        {
            return dot(x, dir) / dot(dir, dir);
        }

        vec4 process(vec2 uv)
        {
            vec4 data = texture(floodfill, uv);

            if (data.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);

            vec2 origin = data.rg;
            vec2 bboxSize = data.ba;

            float radius = length(bboxSize) * 0.5;

            // Per-island random rotation from origin seed
            float rotRand = _rand(vec2(_seed) + origin * vec2(0.01));
            float addedRot = rotRand * 360.0 * prop_variation;

            vec2 dir = buildRot(prop_angle + addedRot) * vec2(-radius, 0);

            // Wrap-aware offset from center
            vec2 offsetFromOrigin = mod(uv - origin + 1.0, 1.0);
            vec2 centerToUv = offsetFromOrigin - bboxSize * vec2(0.5);

            float grad = distAlongDir(centerToUv, dir);
            grad = grad * 0.5 + 0.5;

            return vec4(vec3(grad), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
