import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class MirrorNode extends GpuDesignerNode {
	public init() {
		this.title = "Mirror";

		this.addInput("image");

		this.addEnumProperty("mode", "Mirror Mode", [
			"Left To Right",
			"Right To Left",
			"Top To Bottom",
			"Bottom To Top"
		]);
		this.addFloatProperty("offset", "Offset", 0.5, 0, 1, 0.01);
		this.addBoolProperty("clamp", "Clamp", true);

		const source = `
        vec4 process(vec2 uv)
        {
            // left to right
            if (prop_mode == 0)
                if (uv.x > prop_offset)
                    uv.x = prop_offset - (uv.x - prop_offset);

            // right to left
            if (prop_mode == 1)
                if (uv.x < prop_offset)
                    uv.x = prop_offset + (prop_offset - uv.x);      

            // bottom to top
            if (prop_mode == 2)
                if (uv.y < prop_offset)
                    uv.y = prop_offset + (prop_offset - uv.y);

            // top to bottom
            if (prop_mode == 3)
                if (uv.y > prop_offset)
                    uv.y = prop_offset - (uv.y - prop_offset);
        

            if (prop_clamp)
                uv = clamp(uv, vec2(0.0), vec2(1.0));
            vec4 col = texture(image, uv);
            return col;
        }
        `;

		this.buildShader(source);
	}
}
