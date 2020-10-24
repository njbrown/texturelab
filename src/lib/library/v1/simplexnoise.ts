import { GpuDesignerNode } from "../../designer/gpudesignernode";

// https://thebookofshaders.com/11/
export class SimplexNoiseNode extends GpuDesignerNode {
	public init() {
		this.title = "Simplex Noise";

		this.addFloatProperty("scale", "Scale", 100, 1, 1000, 0.01);

		const source = `
        float random (in vec2 st) {
            return fract(sin(dot(st.xy,
                                 vec2(12.9898,78.233)))
                         * 43758.5453123);
        }

        float noise (in vec2 st) {
            vec2 i = floor(st);
            vec2 f = fract(st);
        
            // Four corners in 2D of a tile
            float a = random(i);
            float b = random(i + vec2(1.0, 0.0));
            float c = random(i + vec2(0.0, 1.0));
            float d = random(i + vec2(1.0, 1.0));
        
            // Smooth Interpolation
        
            // Cubic Hermine Curve.  Same as SmoothStep()
            vec2 u = f*f*(3.0-2.0*f);
            // u = smoothstep(0.,1.,f);
        
            // Mix 4 coorners porcentages
            return mix(a, b, u.x) +
                    (c - a)* u.y * (1.0 - u.x) +
                    (d - b) * u.x * u.y;
        }

        vec2 skew (vec2 st) {
            vec2 r = vec2(0.0);
            r.x = 1.1547*st.x;
            r.y = st.y+0.5*r.x;
            return r;
        }
        
        vec3 simplexGrid (vec2 st) {
            vec3 xyz = vec3(0.0);
        
            vec2 p = fract(skew(st));
            if (p.x > p.y) {
                xyz.xy = 1.0-vec2(p.x,p.y-p.x);
                xyz.z = p.y;
            } else {
                xyz.yz = 1.0-vec2(p.x-p.y,p.y);
                xyz.x = p.x;
            }
        
            return fract(xyz);
        }

        vec4 process(vec2 uv)
        {
            vec3 color = vec3(noise(uv * prop_scale));

            return vec4(color,1.0);
        }
        `;

		this.buildShader(source);
	}
}
