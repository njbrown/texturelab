import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class Rays extends GpuDesignerNode {
	public init() {
		this.title = "Ray";

        this.addIntProperty("sides", "Side", 3, 1, 10, 1);
        this.addFloatProperty("angle", "Angle", 0, 0.0, 360.0, 1);
        this.addFloatProperty("translateX", "Translate X", 0.5, 0.0, 1.0, 0.01);
		this.addFloatProperty("translateY", "Translate Y", 0.5, 0.0, 1.0, 0.01);

		const source = `
        vec4 process(vec2 uv)
        {
            #define PI 3.14159265359

            uv = uv - vec2(prop_translateX,prop_translateY);

            vec2 p = vec2(0) - uv;
            float r = length(p);
            float a = atan(uv.x,uv.y)+radians(prop_angle);
            float n = float(prop_sides);    // n, number of sides
            float shape = sin(r*PI)*0.5+0.5;
                    shape = sin(a*n);
            return vec4(vec3(shape),1.0);
        }
        `;

		this.buildShader(source);
	}
}