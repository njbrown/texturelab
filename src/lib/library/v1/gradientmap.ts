import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";
import { Gradient } from "@/lib/designer/gradient";

export class GradientMapNode extends GpuDesignerNode {
	public init() {
		this.title = "GradientMap";

		this.addInput("inputImage");

		this.addGradientProperty("gradient", "Gradient", Gradient.default());

		const source = `
        float grayscale(vec3 col)
        {
            return (col.r + col.g + col.b) / 3.0;
        }

        vec4 process(vec2 uv)
        {
            // grayscale input color
            float t = grayscale(texture(inputImage, uv).rgb);
            vec3 col = sampleGradient(prop_gradient, t);
            
            return vec4(col, 1.0);
        }
          `;

		this.buildShader(source);
	}
}
