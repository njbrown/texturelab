#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void AmbientOcclusionNode::init()
{
    this->title = "Ambient Occlusion";
    this->addInput("height");

    this->addFloatProp("radius", "Radius", 0.05, 0.001, 1.0, 0.005);
    this->addIntProp("samples", "Samples", 64, 4, 64, 4);
    this->addFloatProp("intensity", "Intensity", 1.0, 0.1, 5.0, 0.1);
    this->addFloatProp("bias", "Bias", 0.01, 0.0, 0.1, 0.005);
    this->addFloatProp("height_scale", "Height Scale", 1.0, 0.01, 1.0, 0.01);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            float centerH = texture(height, uv).r * prop_height_scale;
            float occlusion = 0.0;
            float sampleCount = float(prop_samples);
            float radius = prop_radius;

            for (int i = 0; i < prop_samples; i++)
            {
                // randomFloat(index) uses _randomStart (per-pixel) + _seed + index
                float angle = randomFloat(i * 2) * 6.28318530718;
                vec2 dir = vec2(cos(angle), sin(angle));
                float dist = (randomFloat(i * 2 + 1) * 0.75 + 0.25) * radius;

                vec2 sampleUV = uv + dir * dist;
                float sampleH = texture(height, sampleUV).r * prop_height_scale;

                // Height difference — higher neighbors block light
                float dh = sampleH - centerH;

                // Scale by inverse distance so closer samples matter more
                float distFactor = 1.0 - (dist / radius);
                float contribution = max(dh - prop_bias, 0.0) * distFactor;

                occlusion += contribution;
            }

            occlusion = occlusion / sampleCount;
            occlusion = 1.0 - clamp(occlusion * prop_intensity, 0.0, 1.0);

            return vec4(vec3(occlusion), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
