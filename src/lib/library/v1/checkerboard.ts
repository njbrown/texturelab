import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";

// https://stackoverflow.com/questions/4694608/glsl-checkerboard-pattern
export class CheckerBoardNode extends GpuDesignerNode {
	public init() {
		this.title = "CheckerBoard";

		this.addFloatProperty("rows", "Rows", 2, 1, 20, 1);
		this.addFloatProperty("columns", "Columns", 2, 1, 20, 1);

		this.addColorProperty("color", "Color", new Color());

		const source = `
        vec4 process(vec2 uv)
        {
            if ((mod(prop_columns*uv.x, 1.0) < 0.5) ^^ (mod(prop_rows*uv.y, 1.0) < 0.5))
            {
                return vec4(prop_color.rgb, 1.0);
            }
            else
            {
                return vec4(1.0, 1.0, 1.0, 1.0);
            }
        }
        `;

		this.buildShader(source);
	}
}
