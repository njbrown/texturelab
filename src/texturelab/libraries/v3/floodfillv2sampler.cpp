#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

#include <QColor>

void FloodFillV2SamplerNode::init()
{
    this->title = "FF Sampler V2";

    this->addInput("floodfill");
    this->addInput("image");
    this->addInput("mask");
    this->addInput("size");
    this->addInput("intensity");

    this->addFloatProp("rot", "Rotation", 0, 0, 360, 0.1);
    this->addFloatProp("rotRand", "Random Rotation", 0, 0, 1.0, 0.01);
    this->addFloatProp("posRand", "Random Position", 0, 0, 1.0, 0.01);
    this->addFloatProp("intensityRand", "Random Intensity", 0, 0, 1.0, 0.01);
    this->addFloatProp("scale", "Scale", 1, 0, 4, 0.1);
    this->addFloatProp("scaleRand", "Scale random", 0, 0, 1, 0.1);

    this->addColorProp("bg", "Background Color", QColor());

    auto source = R""""(
        mat3 transMat(vec2 t)
        {
            return mat3(vec3(1.0,0.0,0.0), vec3(0.0,1.0,0.0), vec3(t, 1.0));
        }

        mat3 scaleMat(vec2 s)
        {
            return mat3(vec3(s.x,0.0,0.0), vec3(0.0,s.y,0.0), vec3(0.0, 0.0, 1.0));
        }

        mat3 rotMat(float rot)
        {
            float r = radians(rot);
            return mat3(vec3(cos(r), -sin(r),0.0), vec3(sin(r), cos(r),0.0), vec3(0.0, 0.0, 1.0));
        }

        vec2 transformUV(vec2 uv, vec2 translate, float rot, vec2 scale)
        {
            mat3 trans = transMat(vec2(0.5, 0.5)) *
                transMat(vec2(translate.x, translate.y)) *
                rotMat(rot) *
                scaleMat(vec2(scale.x, scale.y)) *
                transMat(vec2(-0.5, -0.5));

            vec3 res = inverse(trans) * vec3(uv, 1.0);
            uv = res.xy;

            return clamp(uv, vec2(0.0), vec2(1.0));
        }

        float randomFloatRange(vec2 seed, int offset, float fmin, float fmax)
        {
            float r = _rand(vec2(_seed) + seed + vec2(float(offset)) * 0.01);
            return fmin + (fmax - fmin) * r;
        }

        vec4 sampleImage(sampler2D img, vec2 uv)
        {
            if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0)
                return texture(img, uv);
            return vec4(prop_bg.rgb, 1.0);
        }

        vec4 process(vec2 uv)
        {
            vec4 data = texture(floodfill, uv);

            if (data.ba == vec2(0.0, 0.0))
                return vec4(prop_bg.rgb, 1.0);

            // Origin and bbox from encoding
            vec2 origin = data.rg;
            vec2 bboxSize = data.ba;
            vec2 center = fract(origin + bboxSize * vec2(0.5));

            // Compute UV within bbox for this pixel (wrap-aware)
            vec2 uvInBbox = mod(uv - origin + 1.0, 1.0) / bboxSize;

            // Per-island random values seeded from origin
            vec2 randOffset = vec2(0.0);
            {
                float rx = randomFloatRange(origin, 4, -1.0, 1.0);
                float ry = randomFloatRange(origin, 5, -1.0, 1.0);
                randOffset = normalize(vec2(rx, ry)) * prop_posRand;
            }

            float rot = randomFloatRange(origin, 3, -180.0, 180.0) * prop_rotRand + prop_rot;

            // Mask
            if (mask_connected) {
                float m = texture(mask, center).r;
                if (m < 0.001)
                    return vec4(prop_bg.rgb, 1.0);
            }

            // Scale
            float s = prop_scale;
            if (size_connected)
                s *= texture(size, center).r;
            float randScale = randomFloatRange(origin, 3, 0.0, 1.0);
            s = mix(s, randScale, prop_scaleRand);

            // Intensity
            float intens = 1.0;
            if (intensity_connected)
                intens *= texture(intensity, center).r;
            float randIntensity = randomFloatRange(origin, 8, 0.0, 1.0);
            intens = mix(intens, randIntensity, prop_intensityRand);

            vec2 finalUv = transformUV(uvInBbox, randOffset, rot, vec2(s));
            vec3 color = texture(image, finalUv).rgb;

            return vec4(color * intens, 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
