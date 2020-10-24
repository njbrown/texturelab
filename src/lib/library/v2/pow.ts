import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class Pow extends GpuDesignerNode {
	public init() {
		this.title = "Pow";

		this.addInput("image");

		this.addFloatProperty("exponent", "Exponent", 1.0, 0.0, 10.0, 0.01);

		const source = `
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);
            a.rgb = pow(a.rgb, vec3(prop_exponent));

            return a;
        }
        `;

		this.buildShader(source);
	}
}
