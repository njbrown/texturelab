import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class StripesNode extends GpuDesignerNode {
	public init() {
		this.title = "Stripes";

		this.addIntProperty("stripes", "Stripes", 10, 0, 30, 1);
		this.addIntProperty("shift", "Shift", 0, 0, 30, 1);
		this.addFloatProperty("width", "Width", 0.5, 0.0, 1.0, 0.01);

		// calculates normal, then warps uv by it
		const source = `
        vec4 process(vec2 uv)
        {
            float fx = uv.x * 3.142 * 2.0 * float(prop_stripes);
            float fy = uv.y * 3.142 * 2.0 * float(prop_shift);
            float wave = sin(fx + fy);

            // bring wave to range 0...1
            wave = wave * 0.5 + 0.5;
            wave = step(prop_width, wave);

            return vec4(vec3(wave), 1.0);
        }
        `;

		this.buildShader(source);
	}
}
