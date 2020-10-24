import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class Quantize extends GpuDesignerNode {
	public init() {
		this.title = "Quantize";

		this.addInput("image");

		this.addIntProperty("steps", "Steps", 12, 2, 256, 1);

		const source = `
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);
            a.rgb = floor(a.rgb * vec3(float(prop_steps))) / vec3(float(prop_steps));

            return a;
        }
        `;

		this.buildShader(source);
	}
}
