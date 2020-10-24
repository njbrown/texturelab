import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class CopyNode extends GpuDesignerNode {
	public init() {
		this.title = "Copy";

		this.addInput("image");
		this.addStringProperty("name", "Name");

		const source = `
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);
            return col;
        }
        `;

		this.buildShader(source);
	}
}
