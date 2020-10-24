import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class MapRangeNode extends GpuDesignerNode {
	public init() {
		this.title = "Map Range";

		this.addInput("color");
		this.addFloatProperty("in_min", "Input Minimum", 0, 0, 1.0, 0.01);
		this.addFloatProperty("in_max", "Input Maximum", 1, 0, 1.0, 0.01);
		this.addFloatProperty("out_min", "Output Minimum", 0, 0, 1.0, 0.01);
		this.addFloatProperty("out_max", "Output Maximum", 1, 0, 1.0, 0.01);

		const source = `
        vec4 process(vec2 uv)
        {
            vec4 col = texture(color,uv);

            // color range coming in
            float inDiff = prop_in_max - prop_in_min;
            col = (col-prop_in_min) / inDiff;


            float outDiff = prop_out_max - prop_out_min;
            col.rgb = prop_out_min + col.rgb * vec3(outDiff);
            return col;
        }
        `;

		this.buildShader(source);
	}
}
