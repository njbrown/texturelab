import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";

export class OutputNode extends GpuDesignerNode {
	public init() {
		this.title = "Output";

		this.addInput("image");
		this.addStringProperty("name", "Output Name");
		this.addEnumProperty("components", "Components", [
			"RGBA",
			"RGB",
			"R",
			"G",
			"B",
			"A"
		]);

		this.addEnumProperty("precision", "Precision", [
			"8 Bits Per Component",
			"16 Bits Per Component"
		]);
		this.addColorProperty("color", "Default Color", new Color());

		const source = `
        vec4 process(vec2 uv)
        {
            vec4 col;
            if (image_connected) {
              col = vec4(0,1,0,1);
              col = texture(image, uv);
            //   return texture(image, uv);
            } else {
              col = prop_color;
            //   return prop_color;
            }

			if(prop_components == 2) col.rgb = vec3(col.r);
			if(prop_components == 3) col.rgb = vec3(col.g);
			if(prop_components == 4) col.rgb = vec3(col.b);
			if(prop_components == 5) col.rgb = vec3(col.a);

            return col;
        }
        `;

		this.buildShader(source);
	}
}
