#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

void ClampNode::init()
{
    this->title = "Clamp";

    this->addInput("inputImage");

    this->addFloatProp("min", "Minimum", 0, 0, 1.0, 0.01);
    this->addFloatProp("max", "Maximum", 1, 0, 1.0, 0.01);

    this->addBoolProp("clamp_r", "Clamp R Chanel", true);
    this->addBoolProp("clamp_g", "Clamp G Chanel", true);
    this->addBoolProp("clamp_b", "Clamp B Chanel", true);
    this->addBoolProp("clamp_a", "Clamp A Chanel", false);

    auto source = R""""(

        vec4 process(vec2 uv)
        {
            // grayscale input color
            vec4 col = texture(inputImage,uv);

            if (prop_clamp_r) col.r = clamp(col.r, prop_min, prop_max);
            if (prop_clamp_g) col.g = clamp(col.g, prop_min, prop_max);
            if (prop_clamp_b) col.b = clamp(col.b, prop_min, prop_max);
            if (prop_clamp_a) col.a = clamp(col.a, prop_min, prop_max);
            
            return col;
        }
          )"""";

    this->setShaderSource(source);
}
