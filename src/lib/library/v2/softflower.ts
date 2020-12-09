import { GpuDesignerNode } from "../../designer/gpudesignernode";

// https://thebookofshaders.com/07/
export class SoftFlower extends GpuDesignerNode {
	public init() {
		this.title = "Soft Flower";

		this.addFloatProperty("radius", "Radius", 0.2, 0, 1.0, 0.01);
		this.addIntProperty("sides", "Sides", 3, 0, 10, 1);
		this.addFloatProperty("depth", "Depth", 1.0, 0, 1.0, 0.01);
		this.addFloatProperty("gradient", "Gradient", 0.1, 0, 1.0, 0.01);

		const source = `
        #define PI 3.14159265359
        #define TWO_PI 6.28318530718

        float linearstep(float a, float b, float t)
        {
            if (t <= a) return 0.0;
            if (t >= b) return 1.0;

            return (t-a)/(b-a);
        }

        // https://www.iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
        // https://www.shadertoy.com/view/3tSGDy
        float sdStar(in vec2 p, in float r, in int n, in float m) // m=[2,n]
        {
            // these 4 lines can be precomputed for a given shape
            float an = PI/float(n);
            float en = PI/m;
            vec2  acs = vec2(cos(an),sin(an));
            vec2  ecs = vec2(cos(en),sin(en));

            // reduce to first sector
            float bn = mod(atan(p.x,p.y),2.0*an) - an;
            p = length(p)*vec2(cos(bn),abs(sin(bn)));

            // line sdf
            p -= r*acs;
            p += ecs*clamp( -dot(p,ecs), 0.0, r*acs.y/ecs.y);
            return length(p)*sign(p.x);
        }

        vec4 process(vec2 uv)
        {   
            uv = uv - vec2(0.5);
            float n = float(prop_sides);  // n, number of sides
            float a = prop_depth;                 // angle factor
            float m = 2.0 + a*a*(n-2.0);        // angle divisor, between 2 and n
            
            float d = sdStar(uv, prop_radius, prop_sides, m);

            vec3 color = vec3(1.0-linearstep(prop_radius-prop_gradient, prop_radius, d));
            //vec3 color = vec3(d);

            return vec4(color, 1.0);
        }
        `;

		this.buildShader(source);
	}
}
