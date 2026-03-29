#include "curvenode.h"

void CurveNode::init()
{
    this->title = "Curve";
    this->addInput("image");
    this->addCurveProp("curve", "Curve");

    auto source = R""""(
        vec4 process(vec2 uv) {
            vec4  col    = texture(image, uv);
            float gray   = (col.r + col.g + col.b) * 0.3333333;
            float mapped = evalCurve(prop_curve, gray);
            return vec4(vec3(mapped), col.a);
        }
    )"""";

    this->setShaderSource(source);
}
