import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";
import { Gradient } from "@/lib/designer/gradient";

export class CartesianToPolar extends GpuDesignerNode {
	public init() {
		this.title = "Grayscale";

        this.addInput("image");

        this.addFloatProperty("yscale", "Y Scale", 1.0, 0, 4.0, 0.01);
        this.addIntProperty("xtile", "X Tile", 2, 0, 5, 1);
        this.addIntProperty("ytile", "Y Tile", 1, 0, 5, 1);
        // this.addBoolProperty("clamp", "Clamp", false);

        const source = `
        const float PI = 3.142;
        // https://stackoverflow.com/questions/26070410/robust-atany-x-on-glsl-for-converting-xy-coordinate-to-angle
        float atan2(in float y, in float x)
        {
            bool s = (abs(x) > abs(y));
            return mix(PI/2.0 - atan(x,y), atan(y,x), s);
        }

        vec4 process(vec2 uv)
        {
            vec2 dir = uv - vec2(0.5, 0.5);
            float y = length(dir) * 2.0 * prop_yscale;
            // if (prop_clamp)
            //     y = min(2.0 * prop_yscale, y);

            //float scaledY = y * prop_yscale;
            //y = max(y, float(prop_ytile));
            // float x = atan(dir.y, dir.x) / (3.142) / float(prop_xtile);
            float x = atan(dir.y, dir.x);
            x = (x + 3.142) / (3.142 * 2.0); // bring to range 0..1
            x = x * float(prop_xtile);

            vec4 col = texture(image, vec2(x, y));

            return col;
        }
          `;

		this.buildShader(source);
	}
}
