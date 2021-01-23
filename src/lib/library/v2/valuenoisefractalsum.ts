import { GpuDesignerNode } from "../../designer/gpudesignernode";

// https://www.shadertoy.com/view/XdXGW8
export class ValueNoiseFractalSum extends GpuDesignerNode {
	public init() {
		this.title = "Value Noise Fractal Sum";

        this.addIntProperty("scale", "Scale", 8, 1, 100, 1);
        this.addIntProperty("layers", "Layers", 5, 1, 20, 1);
        this.addFloatProperty("gain", "Gain", 0.5, 0.1, 2, 0.1);
        this.addFloatProperty("lacunarity", "Lacunarity", 0.5, 0.1, 2, 0.1);
        

        const source = `
        float wrapAround(float value, float upperBound) {
            return mod((value + upperBound - 1.0), upperBound);
        }

        float wrapAndHash(vec2 value, vec2 upperBounds) {
            value.x = wrapAround(value.x, upperBounds.x);
            value.y = wrapAround(value.y, upperBounds.y);

            return -1.0 + 2.0 * hash12(value + vec2(_seed));
        }

        float noise( in vec2 p )
        {
            vec2 i = floor( p );
            vec2 f = fract( p );
            
            vec2 u = f*f*(3.0-2.0*f);

            vec2 bounds = vec2(float(prop_scale));

            return mix( mix( wrapAndHash( i + vec2(0.0,0.0), bounds), 
                            wrapAndHash( i + vec2(1.0,0.0), bounds), u.x),
                        mix( wrapAndHash( i + vec2(0.0,1.0), bounds), 
                            wrapAndHash( i + vec2(1.0,1.0), bounds), u.x), u.y);
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
        `;

		this.buildShader(source);
	}
}
