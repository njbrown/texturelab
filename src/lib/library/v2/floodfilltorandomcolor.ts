import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class FloodFillToRandomColor extends GpuDesignerNode {
	public init() {
		this.title = "Flood Fill To Random Color";

		this.addInput("floodfill");

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

            //uv = floor(uv * 255.0) / 255.0;
            vec2 center = calcFloodFillCenter(uv, pixelData);
            // vec2 center = calcFloodFillOrigin(uv, pixelData);
            // _rand(vec2(_seed) + center * vec2(0.01));
            //_rand(vec2(_seed) + (brickId + vec2(1) ) * vec2(0.01))

            // quantize center to remove minor innaccuracies
            // the hash function is very sensitive to even small changes
            center = floor(center * 10.0) / 10.0;
            
            vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
            color.r = _rand(center + (vec2(1) ) * vec2(0.01));
            color.g = _rand(center + (vec2(2) ) * vec2(0.01));
            color.b = _rand(center + (vec2(3) ) * vec2(0.01));

            // color.r = _rand(center );
            // color.g = _rand(center );
            // color.b = _rand(center );
            // color.rgb = vec3(_rand(center * vec2(0.01)));
            //color.rgb = vec3(hash12(center));
            //color.rg = center;
            
            

            //color.rgb = vec3(hash12(pixelData.ba));
            // color.rgb = vec3(hash12(center));

            return color;
        }
        `;

		this.buildShader(source);
	}
}
