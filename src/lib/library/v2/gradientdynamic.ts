import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class GradientDynamic extends GpuDesignerNode {
	public init() {
		this.title = "Gradient Dynamic";

		this.addInput("image");
		this.addInput("gradient");

		this.addFloatProperty("position", "Position", 0, 0, 1, 0.01);

		this.addEnumProperty("orientation", "Gradient Direction", [
			"Vertical",
			"Horizontal"
		]);

		const source = `
        vec4 process(vec2 uv)
        {
            float t = 0.0;
    
            float intensity = texture(image, uv).r;

            vec4 col = vec4(0);
            if (prop_orientation == 0)
                col = texture(gradient, vec2(prop_position, intensity));
            else
                col = texture(gradient, vec2(intensity, prop_position));
            
            return col;
        }
        `;

		this.buildShader(source);
	}
}
