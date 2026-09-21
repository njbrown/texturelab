#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// https://en.wikipedia.org/wiki/Sobel_operator
// https://docs.opencv.org/4.x/d2/d2c/tutorial_sobel_derivatives.html
void EdgeDetectNode::init()
{
    this->title = "Edge Detect";

    this->addInput("image");

    auto kernelProp = this->addEnumProp("kernel", "Kernel",
                                        {"Sobel", "Prewitt", "Laplacian"});
    kernelProp->index = 0;

    this->addFloatProp("intensity",  "Intensity",  1.0, 0.0, 4.0, 0.1);
    this->addFloatProp("threshold",  "Threshold",  0.0, 0.0, 1.0, 0.01);
    this->addFloatProp("width",      "Width",      1.0, 0.5, 8.0, 0.5);
    this->addBoolProp ("invert",     "Invert",     false);

    auto source = R""""(
        #define KERNEL_SOBEL     0
        #define KERNEL_PREWITT   1
        #define KERNEL_LAPLACIAN 2

        float luminance(vec3 c) {
            return dot(c, vec3(0.2126, 0.7152, 0.0722));
        }

        float sampleLum(vec2 uv) {
            return luminance(texture(image, uv).rgb);
        }

        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.0, 0.0, 0.0, 1.0);

            vec2 step = (prop_width / _textureSize.xy);

            float tl = sampleLum(uv + vec2(-step.x,  step.y));
            float t  = sampleLum(uv + vec2( 0.0,     step.y));
            float tr = sampleLum(uv + vec2( step.x,  step.y));
            float l  = sampleLum(uv + vec2(-step.x,  0.0));
            float r  = sampleLum(uv + vec2( step.x,  0.0));
            float bl = sampleLum(uv + vec2(-step.x, -step.y));
            float b  = sampleLum(uv + vec2( 0.0,    -step.y));
            float br = sampleLum(uv + vec2( step.x, -step.y));

            float gx = 0.0;
            float gy = 0.0;
            float edge = 0.0;

            if (prop_kernel == KERNEL_SOBEL) {
                gx = -tl - 2.0*l - bl + tr + 2.0*r + br;
                gy = -tl - 2.0*t - tr + bl + 2.0*b + br;
                edge = length(vec2(gx, gy));
            } else if (prop_kernel == KERNEL_PREWITT) {
                gx = -tl - l - bl + tr + r + br;
                gy = -tl - t - tr + bl + b + br;
                edge = length(vec2(gx, gy));
            } else {
                // Laplacian: [-1,-1,-1; -1,8,-1; -1,-1,-1] × center
                float center = sampleLum(uv);
                edge = abs(8.0 * center - (tl + t + tr + l + r + bl + b + br));
            }

            edge *= prop_intensity;
            edge = max(0.0, edge - prop_threshold);

            if (prop_invert)
                edge = 1.0 - clamp(edge, 0.0, 1.0);
            else
                edge = clamp(edge, 0.0, 1.0);

            return vec4(vec3(edge), 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
