import { Color } from "@/lib/designer/color";
import { GpuDesignerNode } from "@/lib/designer/gpudesignernode";

export class Layout2x2 extends GpuDesignerNode {
	public init() {
		this.title = "Layout2x2";

		this.addInput("image0");
		this.addInput("image1");
		this.addInput("image2");
		this.addInput("image3");

		this.addColorProperty("bg", "Background Color", new Color());

		const source = `

		vec4 process(vec2 uv)
		{
            vec2 cellSize = vec2(0.5, 0.5);
            vec2 cell = floor(uv / cellSize);
            vec2 subUv = (uv - cell * cellSize) / cellSize;

            vec4 tex0 = texture(image0, subUv);
            vec4 tex1 = texture(image1, subUv);
            vec4 tex2 = texture(image2, subUv);
            vec4 tex3 = texture(image3, subUv);

            if(image0_connected) {
                if(cell.x < 1.0 && cell.y > 0.0) {
                    return tex0;
                }
            }
            if(image1_connected) {
                if(cell.x > 0.0 && cell.y > 0.0) {
                    return tex1;
                }
            }
            if(image2_connected) {
                if(cell.x < 1.0 && cell.y < 1.0) {
                    return tex2;
                }
            }
            if(image3_connected) {
                if(cell.x > 0.0 && cell.y < 1.0) {
                    return tex3;
                }
            }

            return vec4(prop_bg.rgb, 1.0);
        }
		`;

		this.buildShader(source);
	}
}
