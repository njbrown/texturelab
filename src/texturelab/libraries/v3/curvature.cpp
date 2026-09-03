#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void CurvatureNode::init()
{
    this->title = "Curvature";
    this->addInput("height");

    auto typeProp = this->addEnumProp("type", "Type",
                      {"Mix", "Sharp", "Medium", "Smooth"});
    typeProp->index = 3;
    
                      this->addFloatProp("intensity", "Intensity", 1.0, 0.0, 2.0, 0.1);
    this->addFloatProp("angle", "Angle", 0.5, -1.0, 1.0, 0.1);
    this->addIntProp("samples", "Samples", 12, 1, 32, 1);

    auto advancedProps = this->createGroup("Advanced");
    advancedProps->add(
        this->addFloatProp("sh_c", "Sharp Curvature", 0.5, 0.0, 1.0, 0.1));
    advancedProps->add(
        this->addFloatProp("me_c", "Medium Curvature", 1.5, 0.0, 3.0, 0.1));
    advancedProps->add(
        this->addFloatProp("sm_c", "Smooth Curvature", 3.0, 0.0, 4.0, 0.1));

    auto source = R""""(
        #define LAPLACIAN_CENTER_WEIGHT 2.0
        #define RADIUS_SCALE 0.01
        #define SHARP_WEIGHT 8.0
        #define MEDIUM_WEIGHT 3.0
        #define SMOOTH_WEIGHT 1.5

        #define TYPE_MIX 0
        #define TYPE_SHARP 1
        #define TYPE_MEDIUM 2
        #define TYPE_SMOOTH 3

        float HeightMap(vec2 p)
        {
            return texture(height, p).x;
        }

        float _curveSample(vec2 p, vec2 o)
        {
            float a = HeightMap(p + o);
            float b = HeightMap(p - o);
            return -a - b;
        }

        float CurvatureMap(vec2 p, float r)
        {
            float q = float(prop_samples);
            float s = r / q;
            float H = HeightMap(p) * LAPLACIAN_CENTER_WEIGHT;
            float v = 0.0;

            for (float ox = -q; ox < q; ox++)
            for (float oy = -q; oy < q; oy++)
            {
                vec2 o = vec2(ox, oy);
                float c = _curveSample(p, o * s);
                v += (H + c) * ((r - length(o * s)) / r);
            }

            return v / (q * q);
        }

        vec4 process(vec2 uv)
        {
            if (!height_connected)
                return vec4(0.0, 0.0, 0.0, 1.0);

            float i = prop_intensity;
            float c = 0.0;

            if (prop_type == TYPE_MIX) {
                c += CurvatureMap(uv, i * prop_sh_c * RADIUS_SCALE) * SHARP_WEIGHT;
                c += CurvatureMap(uv, i * prop_me_c * RADIUS_SCALE) * MEDIUM_WEIGHT;
                c += CurvatureMap(uv, i * prop_sm_c * RADIUS_SCALE) * SMOOTH_WEIGHT;
            } else if (prop_type == TYPE_SHARP) {
                c += CurvatureMap(uv, i * prop_sh_c * RADIUS_SCALE) * SHARP_WEIGHT;
            } else if (prop_type == TYPE_MEDIUM) {
                c += CurvatureMap(uv, i * prop_me_c * RADIUS_SCALE) * MEDIUM_WEIGHT;
            } else if (prop_type == TYPE_SMOOTH) {
                c += CurvatureMap(uv, i * prop_sm_c * RADIUS_SCALE) * SMOOTH_WEIGHT;
            }

            vec4 color;
            color.rgb = vec3(prop_angle + c);
            color.a = 1.0;

            return color;
        }
    )"""";

    this->setShaderSource(source);
}
