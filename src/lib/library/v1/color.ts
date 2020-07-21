import { DesignerNode } from "../../designer/designernode";
import { Color } from "@/lib/designer/color";

export class ColorizeNode extends DesignerNode {
	public init() {
		this.title = "Colorize";

		this.addInput("image");

		this.addColorProperty("color", "Color", new Color());

		var source = `
        vec4 process(vec2 uv)
        {
            return texture(image,uv) * prop_color;
        }
        `;

		this.buildShader(source);
	}
}

export class ColorNode extends DesignerNode {
	public init() {
		this.title = "Color";

		this.addColorProperty("color", "Color", new Color());

		var source = `
        vec4 process(vec2 uv)
        {
            return prop_color;
        }
        `;

		this.buildShader(source);
	}
}
