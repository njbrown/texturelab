import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";
import { Gradient } from "@/lib/designer/gradient";

export class Grayscale extends GpuDesignerNode {
	public init() {
		this.title = "Grayscale";

		this.addInput("image");

		const source = `
        vec4 process(vec2 uv)
        {
            // grayscale input color
            vec4 col = texture(image, uv);
			
			col.rgb = vec3((col.r + col.g + col.b) * 0.3333333);

            return col;
        }
          `;

		this.buildShader(source);
	}
}
