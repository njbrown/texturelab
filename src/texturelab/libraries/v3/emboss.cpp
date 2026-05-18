#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// https://en.wikipedia.org/wiki/Emboss_(photography)
// Fake 2D directional light across a height-map surface.
// Distinct from Normal Map: outputs a shaded image, not a vector map.
// Use as a quick height-preview or for stylised cel-shading effects.
void EmbossNode::init()
{
    this->title = "Emboss";

    this->addInput("image");

    this->addFloatProp("angle",     "Light Angle",      45.0, 0.0,  360.0, 1.0);
    this->addFloatProp("elevation", "Light Elevation",   0.5, 0.0,    1.0, 0.05);
    this->addFloatProp("intensity", "Intensity",         3.0, 0.1,   16.0, 0.1);
    this->addBoolProp ("invert",    "Invert Height",    false);

    auto source = R""""(
        #define PI 3.14159265358979

        float sampleH(vec2 uv) {
            float h = texture(image, uv).r;
            return prop_invert ? 1.0 - h : h;
        }

        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.5, 0.5, 0.5, 1.0);

            float angleRad = prop_angle * PI / 180.0;
            vec2  texel    = 1.0 / _textureSize.xy;

            // Central-difference gradient
            float hL = sampleH(uv - vec2(texel.x, 0.0));
            float hR = sampleH(uv + vec2(texel.x, 0.0));
            float hD = sampleH(uv - vec2(0.0, texel.y));
            float hU = sampleH(uv + vec2(0.0, texel.y));

            // Build surface normal from finite differences
            // intensity scales how steeply height maps to angle
            vec3 normal = normalize(vec3(
                (hL - hR) * prop_intensity,
                (hD - hU) * prop_intensity,
                1.0
            ));

            // Light direction from angle and elevation
            float elev    = prop_elevation * PI * 0.5; // [0..PI/2]
            vec3 lightDir = normalize(vec3(
                cos(angleRad) * cos(elev),
                sin(angleRad) * cos(elev),
                sin(elev)
            ));

            float diffuse = dot(normal, lightDir);
            float result  = diffuse * 0.5 + 0.5; // remap [-1,1] → [0,1]

            return vec4(vec3(clamp(result, 0.0, 1.0)), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
