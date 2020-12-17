import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class FloodFillToColor extends GpuDesignerNode {
	public init() {
		this.title = "Flood Fill To Color";

		this.addInput("floodfill");
		this.addInput("color");

		this.addIntProperty("variance", "Variance", 30, 0, 60, 1);

		const source = `
        vec2 calcFloodFillOrigin(vec2 uv, vec4 pixelData)
        {
            //     pixelPos      box width       pixel uv to box
            return   uv    -     pixelData.ba  *  pixelData.rg;
        }

        vec2 calcFloodFillCenter(vec2 uv, vec4 pixelData)
        {
            vec2 origin = calcFloodFillOrigin(uv, pixelData);
            origin += pixelData.ba * vec2(0.5);

            return origin;
        }

        float wrapAround(float value, float upperBound) {
            return mod((value + upperBound - 1.0), upperBound);
        }

        vec4 process(vec2 uv)
        {
            vec4 pixelData =  texture(floodfill, uv);
            if (pixelData.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);
            vec2 center = calcFloodFillCenter(uv, pixelData);

            // convert to local
            center.x = wrapAround(center.x, 1.0);
            center.y = wrapAround(center.y, 1.0);

            // quantize center to remove minor innaccuracies
            // the hash function is very sensitive to even small changes
            float variance = float(prop_variance);
            center = floor(center * variance) / variance;
            
            vec4 color = texture(color, center);

            return color;
        }
        `;

		this.buildShader(source);
	}
}
