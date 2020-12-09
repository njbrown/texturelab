import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class FloodFillToRandomIntensity extends GpuDesignerNode {
	public init() {
		this.title = "Flood Fill To Random Intensity";

		this.addInput("floodfill");
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

        vec4 process(vec2 uv)
        {
            vec4 pixelData =  texture(floodfill, uv);
            if (pixelData.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);

            vec2 center = calcFloodFillCenter(uv, pixelData);

            // quantize center to remove minor innaccuracies
            // the hash function is very sensitive to even small changes
            float variance = float(prop_variance);
            center = floor(center * variance) / variance;
            
            vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
            color.rgb = vec3(_rand(center + (vec2(1) ) * vec2(0.01)));

            return color;
        }
        `;

		this.buildShader(source);
	}
}
