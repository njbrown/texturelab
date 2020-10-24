import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class InvertNode extends GpuDesignerNode {
	public init() {
		this.title = "Invert";

		this.addInput("color");

		const source = `
        vec4 process(vec2 uv)
        {
            vec4 col = vec4(1.0) - texture(color,uv);
            col.a = 1.0;
            return col;
        }
        `;

		this.buildShader(source);
	}
}
