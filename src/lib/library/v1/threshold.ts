import { DesignerNode } from "../../designer/designernode";

export class ThresholdNode extends DesignerNode {
	public init() {
		this.title = "Threshold";

		this.addInput("image");

		this.addFloatProperty("threshold", "Threshold", 0.0, 0.0, 1.0, 0.01);
		this.addBoolProperty("invert", "Invert", true);

		var source = `
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);

            if (prop_invert)
                a.rgb = step(1.0 - prop_threshold, a.rgb);
            else
                a.rgb = step(prop_threshold, a.rgb);

            return a;
        }
        `;

		this.buildShader(source);
	}
}
