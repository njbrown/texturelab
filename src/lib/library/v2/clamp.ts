import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";
import { Gradient } from "@/lib/designer/gradient";

export class Clamp extends GpuDesignerNode {
	public init() {
		this.title = "Clamp";

		this.addInput("inputImage");

		this.addFloatProperty("min", "Minimum", 0, 0, 1.0, 0.01);
		this.addFloatProperty("max", "Maximum", 1, 0, 1.0, 0.01);

		this.addBoolProperty("clamp_r", "Clamp R Chanel", true);
		this.addBoolProperty("clamp_g", "Clamp G Chanel", true);
		this.addBoolProperty("clamp_b", "Clamp B Chanel", true);
		this.addBoolProperty("clamp_a", "Clamp A Chanel", false);

		const source = `

        vec4 process(vec2 uv)
        {
            // grayscale input color
            vec4 col = texture(inputImage,uv);

            if (prop_clamp_r) col.r = clamp(col.r, prop_min, prop_max);
            if (prop_clamp_g) col.g = clamp(col.g, prop_min, prop_max);
            if (prop_clamp_b) col.b = clamp(col.b, prop_min, prop_max);
            if (prop_clamp_a) col.a = clamp(col.a, prop_min, prop_max);
            
            return col;
        }
          `;

		this.buildShader(source);
	}
}
