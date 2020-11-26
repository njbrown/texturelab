import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";
import { Gradient } from "@/lib/designer/gradient";

export class ExtractChannel extends GpuDesignerNode {
	public init() {
		this.title = "Extract Channel";

		this.addInput("image");

		// Red
		let prop = this.addEnumProperty("channel", "Channel", [
			"Red",
			"Green",
			"Blue",
			"Alpha",
			"Average (RGB)"
		]);

		const source = `

		float getChannel(vec4 inputData, int mode)
		{
			if (mode == 0) return inputData.r;
			if (mode == 1) return inputData.g;
			if (mode == 2) return inputData.b;
			if (mode == 3) return inputData.a;
			if (mode == 4) {
				return (inputData.r + inputData.g + inputData.b) * 0.3333333;
			}

			return 0.0;
		}

        vec4 process(vec2 uv)
        {
            // grayscale input color
            vec4 col = texture(image, uv);

			float result = 0.0;
			if (image_connected)
				result = getChannel(col, prop_channel);
            
            return vec4(result);
        }
          `;

		this.buildShader(source);
	}
}
