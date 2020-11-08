import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class FloodFillSampler extends GpuDesignerNode {
	public init() {
		this.title = "Flood Fill Sampler";

        this.addInput("floodfill");
        this.addInput("image");
        
        // this.addEnumProperty("sizeMode", "sizeMode", [
		// 	"max(x,y)",
		// 	"min(x,y)",
		// 	"x",
		// 	"y",
		// 	"length(x,y)"
		// ]);

        const source = `
        vec4 process(vec2 uv)
        {
            vec4 pixelData = texture(floodfill, uv);
            if (pixelData.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);

            vec4 color = texture(image, pixelData.rg);

            return color;
        }
        `;

		this.buildShader(source);
	}
}
