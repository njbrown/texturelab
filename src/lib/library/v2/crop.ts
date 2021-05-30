import { GpuDesignerNode } from "@/lib/designer/gpudesignernode";

export class Crop extends GpuDesignerNode {
	public init() {
		this.title = "Crop";

		this.addInput("image");

		this.addFloatProperty("min_x", "Min X", 0.0, 0, 1, 0.01);
		this.addFloatProperty("min_y", "Min Y", 0.0, 0, 1, 0.01);
		this.addFloatProperty("max_x", "Max X", 1.0, 0, 1, 0.01);
		this.addFloatProperty("max_y", "Max Y", 1.0, 0, 1, 0.01);

		const source = `
		
		vec4 process(vec2 uv)
		{
			if (prop_min_x <= uv.x && uv.x <= prop_max_x &&
			    prop_min_y <= uv.y && uv.y <= prop_max_y)
			{
				return texture(image, uv);
			} else {
				return vec4(0,0,0,1);
			}
		}
		`;

		this.buildShader(source);
	}
}
