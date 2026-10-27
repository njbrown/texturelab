// Ported from src/texturelab/libraries/v2/valuenoise.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class ValueNoiseNode extends TextureNode {
    override init(): void {
        this.title = "Value Noise";
        this.addIntProp("scale", "Scale", 100, 1, 1000, 1);
        this.addIntProp("scaleX", "Scale X", 1, 1, 5, 1);
        this.addIntProp("scaleY", "Scale Y", 1, 1, 5, 1);
        const source = /* glsl */ `
        float wrapAround(float value, float upperBound) {
            return mod((value + upperBound - 1.0), upperBound);
        }

        float wrapAndHash(vec2 value, vec2 upperBounds) {
            value.x = wrapAround(value.x, upperBounds.x);
            value.y = wrapAround(value.y, upperBounds.y);

            return hash12(value + vec2(_seed));
        }

        float noise( in vec2 p )
        {
            vec2 i = floor( p );
            vec2 f = fract( p );
            
            vec2 u = f*f*(3.0-2.0*f);

            vec2 bounds = vec2(float(prop_scaleX), float(prop_scaleY))  * float(prop_scale);

            return mix( mix( wrapAndHash( i + vec2(0.0,0.0), bounds), 
                            wrapAndHash( i + vec2(1.0,0.0), bounds), u.x),
                        mix( wrapAndHash( i + vec2(0.0,1.0), bounds), 
                            wrapAndHash( i + vec2(1.0,1.0), bounds), u.x), u.y);
        }

        vec4 process(vec2 uv)
        {
            vec3 color = vec3(noise(uv * vec2(float(prop_scaleX), float(prop_scaleY)) * float(prop_scale)));

            return vec4(color,1.0);
        }
        `;
        this.setShaderSource(source);
    }
}
