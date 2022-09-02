import { GpuDesignerNode } from "../../designer/gpudesignernode";

//https://www.shadertoy.com/view/Xscyzn
export class Swirl extends GpuDesignerNode {
	public init() {
		this.title = "Swirl";

		this.addInput("image");

        this.addFloatProperty("radius", "Radius", 0.7, 0, 1, 0.01);
		this.addFloatProperty("angle", "Angle", 90, 0.0, 360.0, 0.1);

		const source = `
        #define PI 3.14159265359

        vec4 process(vec2 uv) //(vec2 uv)
        {
            if (!image_connected)
                return vec4(0,0,0,1.0);

            float effectRadius = prop_radius; 
            float effectAngle = radians(prop_angle);
            
            vec2 center = vec2(0) - uv;
            center = center == vec2(0., 0.) ? vec2(.5, .5) : center;

            uv = uv - vec2(1) - center;

            float len = length(uv);
            float angle = atan(uv.y, uv.x) + effectAngle * smoothstep(effectRadius, 0.0, len);
            float radius = length(uv);

            return texture(image, vec2(radius * cos(angle), radius * sin(angle)) + center);
        }
        `;

		this.buildShader(source);
	}
}
