#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

// https://www.shadertoy.com/view/XdXGW8
void GradientNoiseNode::init()
{
    this->title = "Gradient Noise";

    this->addIntProp("scale", "Scale", 20, 1, 1000, 1);
    this->addIntProp("scaleX", "Scale X", 1, 1, 5, 1);
    this->addIntProp("scaleY", "Scale Y", 1, 1, 5, 1);

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

            vec2 bounds = vec2(float(prop_scaleX), float(prop_scaleY))  * float(prop_scale);

            return mix( mix( dot( wrapAndHash( i + vec2(0.0,0.0), bounds ), f - vec2(0.0,0.0) ), 
                     dot( wrapAndHash( i + vec2(1.0,0.0), bounds ), f - vec2(1.0,0.0) ), u.x),
                mix( dot( wrapAndHash( i + vec2(0.0,1.0), bounds ), f - vec2(0.0,1.0) ), 
                     dot( wrapAndHash( i + vec2(1.0,1.0), bounds ), f - vec2(1.0,1.0) ), u.x), u.y);
        }

        vec4 process(vec2 uv)
        {
            float f  = noise(uv * vec2(float(prop_scaleX), float(prop_scaleY)) * float(prop_scale));
            
            // adjust range
            f = 0.5 + 0.5 * f;
            
            vec3 color = vec3(f);

            return vec4(color,1.0);
        }
        )"""";

    this->setShaderSource(source);
}
