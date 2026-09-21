#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// Screen-space-style ambient occlusion over a heightfield.
//
// Vogel, "A better way to construct the sunflower head," Math. Biosciences 1979
//   (golden-angle disc — the sample distribution used below)
// https://blog.demofox.org/2017/05/29/when-random-numbers-are-too-random-low-discrepancy-sequences/
//
// Occlusion is estimated as the mean *elevation angle* of the neighbourhood,
// not the mean height difference: sin(atan(dh/dist)) is bounded in [0,1] per
// sample and correctly scale-aware, so a bump close by occludes far more than
// the same bump at the edge of the radius.
//
// For the high-quality variant (mip-pyramid cone taps, progressive
// accumulation, two-scale detail, bilateral denoise) see fastao.cpp.
void AmbientOcclusionNode::init()
{
    this->title = "Ambient Occlusion";
    this->addInput("height");

    this->addFloatProp("radius", "Radius", 0.05, 0.001, 1.0, 0.005);
    this->addIntProp("samples", "Samples", 100, 4, 128, 4);
    this->addFloatProp("intensity", "Intensity", 1.0, 0.1, 5.0, 0.1);
    this->addFloatProp("bias", "Bias", 0.02, 0.0, 0.5, 0.005);
    this->addFloatProp("height_scale", "Height Scale", 1.0, 0.01, 4.0, 0.01);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            float centerH = texture(height, uv).r * prop_height_scale;
            float sampleCount = float(prop_samples);
            float radius = prop_radius;

            // Radius is expressed in UV, so on a non-square texture the
            // sampling disc would be an ellipse in texel space. Scale x to
            // keep it circular; a no-op when the texture is square.
            vec2 aspect = vec2(_textureSize.y / _textureSize.x, 1.0);

            // A single random per pixel rotates the whole spiral. This
            // decorrelates neighbouring pixels without reintroducing the
            // clumping that per-sample white noise causes.
            float rot = randomFloat(0) * 6.28318530718;

            float occlusion = 0.0;
            float weightSum = 0.0;

            for (int i = 0; i < prop_samples; i++)
            {
                // Vogel disc: golden-angle spiral. Deterministic and evenly
                // stratified, so variance falls off far faster than with the
                // white-noise angle/distance pair it replaces.
                float t = (float(i) + 0.5) / sampleCount;
                float angle = float(i) * 2.39996322973 + rot;
                // sqrt(t) makes the taps uniform over the disc *area* rather
                // than over the radius, which otherwise over-samples the centre.
                float dist = sqrt(t) * radius;

                vec2 dir = vec2(cos(angle), sin(angle)) * aspect;

                // fract() wraps the tap: node textures are CLAMP_TO_EDGE, so
                // without this the AO breaks the seam on a tiling texture.
                float sampleH =
                    texture(height, fract(uv + dir * dist)).r * prop_height_scale;

                float dh = sampleH - centerH;

                // sin(atan(dh / dist)) — the elevation angle of this neighbour.
                float sinElev = dh * inversesqrt(dh * dh + dist * dist);

                // Smooth window over the disc edge, so a feature crossing the
                // radius boundary fades in instead of popping.
                float w = 1.0 - t;

                occlusion += clamp(sinElev - prop_bias, 0.0, 1.0) * w;
                weightSum += w;
            }

            occlusion = occlusion / max(weightSum, 0.0001);
            occlusion = 1.0 - clamp(occlusion * prop_intensity, 0.0, 1.0);

            return vec4(vec3(occlusion), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
