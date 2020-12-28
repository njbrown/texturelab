import { GpuDesignerNode } from "../../designer/gpudesignernode";

// https://thebookofshaders.com/07/
export class CapsuleNode extends GpuDesignerNode {
	public init() {
		this.title = "Capsule";

		this.addFloatProperty("radius", "Radius", 0.2, 0, 1.0, 0.01);
		this.addFloatProperty("topRadius", "Top Radius", 0.0, 0.0, 0.5, 0.01);
		this.addFloatProperty("bottomRadius", "Bottom Radius", 0.0, 0, 0.5, 0.01);
		this.addFloatProperty("length", "Length", 0.3, 0, 2.0, 0.01);
		this.addFloatProperty("gradient", "Gradient", 0, 0, 1.0, 0.01);

		const source = `
        #define PI 3.14159265359
        #define TWO_PI 6.28318530718

        float linearstep(float a, float b, float t)
        {
            if (t <= a) return 0.0;
            if (t >= b) return 1.0;

            return (t-a)/(b-a);
        }

        // https://iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
        float sdUnevenCapsule( vec2 p, float r1, float r2, float h )
        {
            p.x = abs(p.x);
            float b = (r1-r2)/h;
            float a = sqrt(1.0-b*b);
            float k = dot(p,vec2(-b,a));
            if( k < 0.0 ) return length(p) - r1;
            if( k > a*h ) return length(p-vec2(0.0,h)) - r2;
            return dot(p, vec2(a,b) ) - r1;
        }

        vec4 process(vec2 uv)
        {
            uv = uv *2.-1.;
            uv.y += prop_length * 0.5;

            float d = sdUnevenCapsule(uv, prop_bottomRadius, prop_topRadius, prop_length);

            vec3 color = vec3(1.0-linearstep(prop_radius-prop_gradient, prop_radius, d));
            //vec3 color = vec3(d);

            return vec4(color, 1.0);
        }
        `;

		this.buildShader(source);
	}
}
