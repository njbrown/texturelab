#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void NormalMapV3Node::init()
{
    this->title = "Normal Map";
    this->addInput("height");

    this->addEnumProp("filter",     "Filter",     {"Simple", "Sobel", "Scharr"});
    this->addEnumProp("channel",    "Channel",    {"Red", "Green", "Blue", "Alpha", "Luminance"});
    this->addFloatProp("strength",  "Strength",   1.0, -4.0, 4.0, 0.05);
    this->addIntProp("range",       "Range",      1, 1, 20, 1);
    this->addBoolProp("res_ind",    "Resolution Independent", false);
    this->addIntProp("ref_res",     "Reference Resolution",   1024, 64, 8192, 64);
    this->addEnumProp("convention", "Convention", {"OpenGL (Y+)", "DirectX (Y-)"});

    auto source = R""""(
        // Extract a single channel from the height input.
        // Channel: 0=R 1=G 2=B 3=A 4=Luminance(BT.709)
        float sampleChannel(vec2 uv)
        {
            vec4 s = texture(height, uv);
            if (prop_channel == 1) return s.g;
            if (prop_channel == 2) return s.b;
            if (prop_channel == 3) return s.a;
            if (prop_channel == 4) return dot(s.rgb, vec3(0.2126, 0.7152, 0.0722));
            return s.r;
        }

        vec4 process(vec2 uv)
        {
            vec2 step = (vec2(1.0) / _textureSize) * float(prop_range);
            if (prop_res_ind)
                step = (vec2(1.0) / float(prop_ref_res)) * float(prop_range);

            // Scale matches V2's effective strength (prop_strength * 0.1 / 2.0).
            // The cross-product construction below naturally incorporates step.x
            // into the z-component, keeping XY and Z in the same magnitude range.
            float scale = prop_strength * 0.05;
            float gx, gy;

            if (prop_filter == 0) {
                // Simple: forward-difference (3-tap), max response = 1.0
                float c = sampleChannel(uv);
                float r = sampleChannel(uv + vec2( step.x,    0.0));
                float u = sampleChannel(uv + vec2(    0.0, step.y));
                gx = r - c;
                gy = u - c;
            } else {
                // 3x3 neighbourhood for Sobel / Scharr
                float tl = sampleChannel(uv + vec2(-step.x,  step.y));
                float tc = sampleChannel(uv + vec2(    0.0,  step.y));
                float tr = sampleChannel(uv + vec2( step.x,  step.y));
                float ml = sampleChannel(uv + vec2(-step.x,     0.0));
                float mr = sampleChannel(uv + vec2( step.x,     0.0));
                float bl = sampleChannel(uv + vec2(-step.x, -step.y));
                float bc = sampleChannel(uv + vec2(    0.0, -step.y));
                float br = sampleChannel(uv + vec2( step.x, -step.y));

                if (prop_filter == 1) {
                    // Sobel — max response = 4.0, normalise so strength is
                    // perceptually equivalent to Simple at the same value
                    gx = (-tl + tr - 2.0*ml + 2.0*mr - bl + br) / 4.0;
                    gy = ( tl + 2.0*tc + tr - bl - 2.0*bc - br) / 4.0;
                } else {
                    // Scharr — max response = 16.0
                    gx = (-3.0*tl + 3.0*tr - 10.0*ml + 10.0*mr - 3.0*bl + 3.0*br) / 16.0;
                    gy = ( 3.0*tl + 10.0*tc + 3.0*tr - 3.0*bl - 10.0*bc - 3.0*br) / 16.0;
                }
            }

            // Cross product of surface tangent vectors — step.x in the z slot keeps
            // the XY/Z ratio consistent regardless of texture resolution or range.
            vec3 dvx = vec3(step.x, 0.0,    gx * scale);
            vec3 dvy = vec3(0.0,    step.y, gy * scale);
            vec3 normal = normalize(cross(dvx, dvy));

            // Convention: OpenGL = Y+ (green-up), DirectX = Y- (green-down)
            if (prop_convention == 1) normal.y = -normal.y;

            return vec4(normal * 0.5 + 0.5, 1.0);
        }
        )"""";

    this->setShaderSource(source);
}
