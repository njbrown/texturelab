#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void AmbientOcclusionNode::init()
{
    this->title = "Ambient Occlusion";
    this->addInput("height");

    this->addFloatProp("radius", "Radius", 0.05, 0.001, 1.0, 0.005);
    this->addIntProp("samples", "Samples", 16, 4, 64, 4);
    this->addFloatProp("intensity", "Intensity", 1.0, 0.1, 5.0, 0.1);
    this->addFloatProp("bias", "Bias", 0.01, 0.0, 0.1, 0.005);
    this->addFloatProp("height_scale", "Height Scale", 0.1, 0.01, 1.0, 0.01);

    auto source = R""""(
        // Hash function for pseudo-random sampling directions
        float hash(vec2 p)
        {
            vec3 p3 = fract(vec3(p.xyx) * 0.1031);
            p3 += dot(p3, p3.yzx + 33.33);
            return fract((p3.x + p3.y) * p3.z);
        }

        vec2 sampleDirection(float i, vec2 uv)
        {
            float angle = hash(uv + i * 7.13) * 6.28318530718;
            return vec2(cos(angle), sin(angle));
        }

        vec4 process(vec2 uv)
        {
            float centerH = texture(height, uv).r * prop_height_scale;
            float occlusion = 0.0;
            float sampleCount = float(prop_samples);
            float radius = prop_radius;

            for (int i = 0; i < prop_samples; i++)
            {
                // Random direction and distance for this sample
                float fi = float(i);
                vec2 dir = sampleDirection(fi, uv);
                float dist = (hash(uv + fi * 3.77) * 0.75 + 0.25) * radius;

                vec2 sampleUV = uv + dir * dist;
                float sampleH = texture(height, sampleUV).r * prop_height_scale;

                // Height difference
                float dh = sampleH - centerH;

                // How much this sample occludes: higher neighbors block light
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
