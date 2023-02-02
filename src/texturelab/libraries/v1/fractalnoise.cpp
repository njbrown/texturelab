#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"

void FractalNoiseNode::init()
{
    this->title = "Fractal Noise";

    this->addIntProp("scale", "Scale", 1, 1, 10, 1);
    this->addIntProp("scaleX", "X Scale", 1, 1, 15, 1);
    this->addIntProp("scaleY", "Y Scale", 1, 1, 15, 1);
    this->addIntProp("startBand", "Start Band", 4, 1, 10, 1);
    this->addIntProp("endBand", "End Band", 8, 1, 10, 1);
    this->addFloatProp("persistence", "Persistence", 0.6, 0.0, 1.0, 0.01);
    this->addFloatProp("persistenceStart", "Persistence Start", 1.0, 0.0, 1.0,
                       0.01);
    this->addFloatProp("brightness", "Brightness", 0.5, 0.0, 1.0, 0.01);

    auto source = R""""(
        //
        // GLSL textureless classic 2D noise "cnoise",
        // with an RSL-style periodic variant "pnoise".
        // Author:  Stefan Gustavson (stefan.gustavson@liu.se)
        // Version: 2011-08-22
        //
        // Many thanks to Ian McEwan of Ashima Arts for the
        // ideas for permutation and gradient selection.
        //
        // Copyright (c) 2011 Stefan Gustavson. All rights reserved.
        // Distributed under the MIT license. See LICENSE file.
        // https://github.com/stegu/webgl-noise
        //
        
        vec4 mod289(vec4 x)
        {
          return x - floor(x * (1.0 / 289.0)) * 289.0;
        }
        
        vec4 permute(vec4 x)
        {
          return mod289(((x*34.0)+1.0)*x);
        }
        
        vec4 taylorInvSqrt(vec4 r)
        {
          return 1.79284291400159 - 0.85373472095314 * r;
        }
        
        vec2 fade(vec2 t) {
          return t*t*t*(t*(t*6.0-15.0)+10.0);
        }

        // Classic Perlin noise, periodic variant
        float pnoise(vec2 P, vec2 rep)
        {
        vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
        vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
        Pi = mod(Pi, rep.xyxy); // To create noise with explicit period
        Pi = mod289(Pi);        // To avoid truncation effects in permutation
        vec4 ix = Pi.xzxz;
        vec4 iy = Pi.yyww;
        vec4 fx = Pf.xzxz;
        vec4 fy = Pf.yyww;

        vec4 i = permute(permute(ix) + iy);

        vec4 gx = fract(i * (1.0 / 41.0)) * 2.0 - 1.0 ;
        vec4 gy = abs(gx) - 0.5 ;
        vec4 tx = floor(gx + 0.5);
        gx = gx - tx;

        vec2 g00 = vec2(gx.x,gy.x);
        vec2 g10 = vec2(gx.y,gy.y);
        vec2 g01 = vec2(gx.z,gy.z);
        vec2 g11 = vec2(gx.w,gy.w);

        vec4 norm = taylorInvSqrt(vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
        g00 *= norm.x;  
        g01 *= norm.y;  
        g10 *= norm.z;  
        g11 *= norm.w;  

        float n00 = dot(g00, vec2(fx.x, fy.x));
        float n10 = dot(g10, vec2(fx.y, fy.y));
        float n01 = dot(g01, vec2(fx.z, fy.z));
        float n11 = dot(g11, vec2(fx.w, fy.w));

        vec2 fade_xy = fade(Pf.xy);
        vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
        float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
        return 2.3 * n_xy;
        }

        vec4 process(vec2 uv)
        {
            float scale_value = prop_persistenceStart;
            float persistence = prop_persistence;
            float total = 0.0;
            vec2 scale = vec2(prop_scaleX, prop_scaleY) * float(prop_scale);
            
            //int startBand = 3;
            //int endBand = 12;
            for(int i = 1; i <=20; i++) {
                if (i >= prop_startBand && i <= prop_endBand) {
                total += (pnoise(uv*scale, scale)) * scale_value;
                scale_value *= persistence;
                }
                scale *= 2.0;
          }

          return vec4(vec3(total + prop_brightness),1.0);
        }
        )"""";

    this->setShaderSource(source);
}