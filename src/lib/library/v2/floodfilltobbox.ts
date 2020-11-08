import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class FloodFillToBBox extends GpuDesignerNode {
	public init() {
		this.title = "Flood Fill To BBox";

		this.addInput("floodfill");

		this.addEnumProperty("function", "Function", [
			"max(x,y)",
			"min(x,y)",
			"x",
			"y",
			"length(x,y)"
		]);

		const source = `
        vec4 process(vec2 uv)
        {
            vec4 pixelData =  texture(floodfill, uv);
            if (pixelData.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);

            float intensity = 0.0;

            if (prop_function==0)
                intensity = max(pixelData.b, pixelData.a);
            else if (prop_function==1)
                intensity = min(pixelData.b, pixelData.a);
            else if (prop_function==2)
                intensity = pixelData.b;
            else if (prop_function==3)
                intensity = pixelData.a;
            else if (prop_function==4)
                intensity = sqrt(pixelData.b * pixelData.b + pixelData.a * pixelData.a);

            return vec4(vec3(intensity), 1.0);
        }
        `;

		this.buildShader(source);
	}
}
