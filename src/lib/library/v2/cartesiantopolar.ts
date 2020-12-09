import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";
import { Gradient } from "@/lib/designer/gradient";

// https://www.mathsisfun.com/polar-cartesian-coordinates.html
export class CartesianToPolar extends GpuDesignerNode {
	public init() {
		this.title = "Cartesian To Polar";

		this.addInput("image");

		this.addFloatProperty("yscale", "Y Scale", 1.0, 0, 4.0, 0.01);
		this.addIntProperty("xtile", "X Tile", 2, 0, 5, 1);
		this.addFloatProperty("angle_offset", "Angle Offset", 0.0, 0, 360, 1.0);
		// this.addBoolProperty("clamp", "Clamp", false);

		const source = `
        const float PI = 3.142;
        vec4 process(vec2 uv)
        {
            vec2 dir = uv - vec2(0.5, 0.5);
            float y = length(dir) * 2.0 * prop_yscale;

            float x = atan(dir.y, dir.x);
            x = (x + PI) / (PI * 2.0); // bring to range 0..1
            x += (prop_angle_offset / 360.0);
            x = x * float(prop_xtile);

            vec4 col = texture(image, vec2(x, y));

            return col;
        }
          `;

		this.buildShader(source);
	}
}
