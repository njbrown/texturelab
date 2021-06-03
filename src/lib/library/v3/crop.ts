import { Color } from "@/lib/designer/color";
import { GpuDesignerNode } from "@/lib/designer/gpudesignernode";

export class Crop extends GpuDesignerNode {
	public init() {
		this.title = "Crop";

		this.addInput("image");

		this.addFloatProperty("min_x", "X Start", 0.0, 0, 1, 0.01);
		this.addFloatProperty("max_x", "X End", 1.0, 0, 1, 0.01);
		this.addFloatProperty("min_y", "Y Start", 0.0, 0, 1, 0.01);
		this.addFloatProperty("max_y", "Y End", 1.0, 0, 1, 0.01);

		this.addColorProperty("bg", "Background Color", new Color());

		this.addBoolProperty("clamp", "Clamp", false);
		this.addBoolProperty("clamp_x", "Clamp X", false);
		this.addBoolProperty("clamp_y", "Clamp Y", false);

		const source = `
		
		bool is_fg(float val, float min_val, float max_val, bool clamp)
		{
			if (min_val <= max_val) {
				return min_val <= val && val <= max_val;
			} else {
				if (clamp) return false;

				return val <= max_val || min_val <= val;
			}
		}

		vec4 process(vec2 uv)
		{
			if (is_fg(uv.x, prop_min_x, prop_max_x, prop_clamp || prop_clamp_x) && is_fg(uv.y, prop_min_y, prop_max_y, prop_clamp || prop_clamp_y))
			{
				return texture(image, uv);
			} else {
				return vec4(prop_bg.rgb, 1.0);
			}
		}
		`;

		this.buildShader(source);
	}
}
