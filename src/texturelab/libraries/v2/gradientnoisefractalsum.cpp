#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

// https://www.shadertoy.com/view/XdXGW8
void GradientNoiseFractalSumNode::init()
{
    this->title = "Gradient Noise Fractal Sum";

    this->addIntProp("scale", "Scale", 8, 1, 100, 1);
    this->addIntProp("layers", "Layers", 5, 1, 20, 1);
    this->addFloatProp("gain", "Gain", 0.5, 0.1, 2, 0.1);
    this->addFloatProp("lacunarity", "Lacunarity", 2.0, 0.1, 2, 0.1);

    auto source = R""""(
        vec2 hash( vec2 x )  // replace this by something better
        {
            const vec2 k = vec2( 0.3183099, 0.3678794 );
            x = x*k + k.yx;
            return -1.0 + 2.0*fract( 16.0 * k*fract( x.x*x.y*(x.x+x.y)) );
        }

        float wrapAround(float value, float upperBound) {
            return mod((value + upperBound - 1.0), upperBound);
        }

        vec2 wrapAndHash(vec2 value, vec2 upperBounds) {
            value.x = wrapAround(value.x, upperBounds.x);
            value.y = wrapAround(value.y, upperBounds.y);

            return -1.0 + 2.0 * hash22(value + vec2(_seed));
        }

        float noise( in vec2 p )
        {
            vec2 i = floor( p );
            vec2 f = fract( p );
            
            vec2 u = f*f*(3.0-2.0*f);

            vec2 bounds = vec2(float(prop_scale));

            return mix( mix( dot( wrapAndHash( i + vec2(0.0,0.0), bounds ), f - vec2(0.0,0.0) ), 
                     dot( wrapAndHash( i + vec2(1.0,0.0), bounds ), f - vec2(1.0,0.0) ), u.x),
                mix( dot( wrapAndHash( i + vec2(0.0,1.0), bounds ), f - vec2(0.0,1.0) ), 
                     dot( wrapAndHash( i + vec2(1.0,1.0), bounds ), f - vec2(1.0,1.0) ), u.x), u.y);
        }

        vec4 process(vec2 uv)
        {

            float scale = float(prop_scale);
            float amplitude = 1.0;

            float sum = 0.0;

            for(int i = 0; i < prop_layers; i++) {
                sum += noise(uv * float(scale)) * amplitude;

                scale *= prop_lacunarity;
                amplitude *= prop_gain;
            }
            
            // adjust range
            sum = 0.5 + 0.5 * sum;
            
            vec3 color = vec3(sum);

            return vec4(color,1.0);
        }
        )"""";

    this->setShaderSource(source);
}
