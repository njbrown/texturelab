#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"

// void GradientMapNode::init()
// {
//     this->title = "GradientMap";

//     this->addInput("inputImage");

//     this->addGradientProp("gradient", "Gradient", Gradient::default());

//     auto source = R""""(
//         float grayscale(vec3 col)
//         {
//             return (col.r + col.g + col.b) / 3.0;
//         }

//         vec4 process(vec2 uv)
//         {
//             // grayscale input color
//             float t = grayscale(texture(inputImage, uv).rgb);
//             vec3 col = sampleGradient(prop_gradient, t);

//             return vec4(col, 1.0);
//         }
//         )"""";

//     this->setShaderSource(source);
// }