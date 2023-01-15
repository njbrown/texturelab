#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

// multiple pass warp
SlopeBlurNode::init()
{
    this->title = "Slope Blur";

    this->addInput("image");
    this->addInput("slope");

    this->addFloatProp("intensity", "Intensity", 1, 0, 5, 0.1);
    this->addIntProp("quality", "Quality", 5, 0, 10, 1);

    auto source = R""""(
        vec2 calcSlope(vec2 uv)
        {
            vec3 sl = vec3(0.0,0.0,0.0);
            sl.x = texture(slope, uv + vec2(0.0, 1.0/_textureSize.y)).r;
            sl.y = texture(slope, uv + vec2(-1.0/_textureSize.x, -0.5/_textureSize.y)).r;
            sl.z = texture(slope, uv + vec2(1.0/_textureSize.x, -0.5/_textureSize.y)).r;
            
            vec2 result = vec2(0.0);
            result.x = sl.z-sl.y;
            result.y = dot(sl, vec3(1, -0.5, -0.5));
            
            return result;
        }

        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0,0,0,1.0);

            float intensity = prop_intensity;
            int iterations = int(ceil(float(prop_quality) * prop_intensity));
            
            vec3 color = vec3(0.0);
            for(int i = 0;i<iterations;i++){
                vec2 slopeValue  = calcSlope(uv) * _textureSize.xy;
                
                // note: the uv isnt reset each iteration because the
                // blur should follow the path of the previous slope
                uv += slopeValue * (intensity / _textureSize.x) * (intensity / float(iterations));
                    
                color += texture(image, uv).rgb;
            }
            
            vec3 final = color / float(iterations);

            return vec4(vec3(final),1.0);
        }
        )"""";

    this->setShaderSource(source);
}