#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

// https://www.shadertoy.com/view/Msf3WH
// previous implementation was actually value noise
void SimplexNoiseV2Node::init()
{
    this->title = "Simplex Noise";

    this->addIntProp("scale", "Scale", 100, 1, 1000, 1);
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

            return hash(value + vec2(_seed));
        }

        float noise( in vec2 p )
        {
            vec2 bounds = vec2(float(prop_scaleX), float(prop_scaleY))  * float(prop_scale);

            const float K1 = 0.366025404; // (sqrt(3)-1)/2;
            const float K2 = 0.211324865; // (3-sqrt(3))/6;

            vec2  i = floor( p + (p.x+p.y)*K1 );
            vec2  a = p - i + (i.x+i.y)*K2;
            float m = step(a.y,a.x); 
            vec2  o = vec2(m,1.0-m);
            vec2  b = a - o + K2;
            vec2  c = a - 1.0 + 2.0*K2;
            vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
            vec3  n = h*h*h*h*vec3( dot(a,wrapAndHash(i+0.0, bounds)), dot(b,wrapAndHash(i+o, bounds)), dot(c,wrapAndHash(i+1.0, bounds)));
            return dot( n, vec3(70.0) );
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