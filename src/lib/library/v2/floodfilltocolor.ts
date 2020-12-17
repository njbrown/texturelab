import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class FloodFillToColor extends GpuDesignerNode {
	public init() {
		this.title = "Flood Fill To Color";

		this.addInput("floodfill");
		this.addInput("color");

		this.addIntProperty("precision", "Precision", 2, 1, 3, 1);

		let prop = this.addEnumProperty("source", "Sample Origin", [
			"Top-Left",
			"Center"
		]);

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

        // https://forum.processing.org/two/discussion/13586/how-to-round-a-float-to-its-second-or-third-decimal
        float floatRound(float number, int place) {
            float rounder = 1.0 / float(place);
            return number - mod(number, rounder);
        }

        vec4 process(vec2 uv)
        {
            vec4 pixelData =  texture(floodfill, uv);
            if (pixelData.ba == vec2(0.0, 0.0))
                return vec4(0.0, 0.0, 0.0, 1.0);

            // sampling origin as opposed to center because of precison
            // issues that comes with the division
            vec2 center;
            if (prop_source == 0)
                center = calcFloodFillOrigin(uv, pixelData);
            else if (prop_source == 1)
                center = calcFloodFillCenter(uv, pixelData);

            // convert to local
            center.x = wrapAround(center.x, 1.0);
            center.y = wrapAround(center.y, 1.0);

            // quantize center to remove minor innaccuracies
            // the hash function is very sensitive to even small changes
            int place = int(pow(float(10), float(prop_precision)));
            center.x = floatRound(center.x, place);
            center.y = floatRound(center.y, place);
            
            vec4 color = texture(color, center);

            return color;
        }
        `;

		this.buildShader(source);
	}
}
