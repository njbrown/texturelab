import { GpuDesignerNode } from "../../designer/gpudesignernode";

// https://thebookofshaders.com/07/
export class PolygonNode extends GpuDesignerNode {
	public init() {
		this.title = "Polygon";

		this.addFloatProperty("radius", "Radius", 0.7, 0, 3, 0.01);
		this.addFloatProperty("angle", "Angle", 0, 0.0, 360.0, 1);
		this.addIntProperty("sides", "Sides", 5, 0, 20, 1);
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

        vec4 process(vec2 uv)
        {
            uv = uv *2.-1.;

            // Angle and radius from the current pixel
            float a = atan(uv.x,uv.y)+radians(prop_angle);
            float r = TWO_PI/float(prop_sides);

            float d = cos(floor(.5+a/r)*r-a)*length(uv) / prop_radius;

            vec3 color = vec3(1.0-linearstep(0.8-prop_gradient,0.8,d));

            return vec4(color, 1.0);
        }
        `;

		this.buildShader(source);
	}
}

export class CircleNode extends GpuDesignerNode {
	public init() {
		this.title = "Circle";

		this.addFloatProperty("radius", "Radius", 0.5, 0, 1, 0.01);
		this.addEnumProperty("color_gen", "Color Generation", [
			"Flat",
			"Linear",
			"Exponent"
		]);

		const source = `
        vec4 process(vec2 uv)
        {
            float dist = distance(uv, vec2(0.5));
            if( dist <= prop_radius) {
                if (prop_color_gen==0)
                    return vec4(vec3(1.0), 1.0);
                else if (prop_color_gen==1)
                    return vec4(vec3(1.0 - dist / prop_radius), 1.0);
                else if (prop_color_gen==2)
                {
                    float val = dist / prop_radius;
                    return vec4(vec3(1.0 - val * val), 1.0);
                }
                
            }

            return vec4(vec3(0.0), 1.0);
        }
        `;

		this.buildShader(source);
	}
}
