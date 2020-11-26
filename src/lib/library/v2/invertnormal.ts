import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";
import { Gradient } from "@/lib/designer/gradient";

export class InvertNormal extends GpuDesignerNode {
	public init() {
		this.title = "Invert Normal";

		this.addInput("inputImage");

		this.addBoolProperty("invertRed", "Invert Red", false);
		this.addBoolProperty("invertGreen", "Invert Green", true);
		this.addBoolProperty("invertBlue", "Invert Blue", false);
		this.addBoolProperty("invertAlpha", "Invert Alpha", false);

		const source = `

        vec4 process(vec2 uv)
        {
            // grayscale input color
            vec4 result = texture(inputImage,uv);

			if(prop_invertRed) result.r = 1.0 - result.r;
			if(prop_invertGreen) result.g = 1.0 - result.g;
			if(prop_invertBlue) result.b = 1.0 - result.b;
			if(prop_invertAlpha) result.a = 1.0 - result.a;
            
            return result;
        }
          `;

		this.buildShader(source);
	}
}
