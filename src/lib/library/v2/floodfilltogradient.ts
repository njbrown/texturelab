import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class FloodFillToGradient extends GpuDesignerNode {
	public init() {
		this.title = "Flood Fill To Gradient";

		this.addInput("floodfill");

		this.addFloatProperty("angle", "Angle", 0, 0, 360, 1);
		this.addFloatProperty("variation", "Angle Variation", 0, 0, 1.0, 0.05);
		this.addIntProperty("precision", "Precision", 2, 1, 3, 1);

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

        mat2 buildRot(float rot)
        {
            float r = radians(rot);
            return mat2(cos(r), -sin(r), sin(r), cos(r));
        }

        // https://github.com/g-truc/glm/blob/master/glm/gtx/projection.inl
        float distAlongDir(vec2 x, vec2 dir)
        {
            return dot(x, dir) / dot(dir, dir);
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

                
            vec2 origin = calcFloodFillOrigin(uv, pixelData);
            vec2 center = calcFloodFillCenter(uv, pixelData);

            vec2 originSeed;
            int place = int(pow(float(10), float(prop_precision)));
            originSeed.x = wrapAround(origin.x, 1.0);
            originSeed.y = wrapAround(origin.y, 1.0);
            originSeed.x = floatRound(originSeed.x, place);
            originSeed.y = floatRound(originSeed.y, place);

            float rotRand = _rand(vec2(_seed) + originSeed * vec2(0.01));
            float addedRot = rotRand * 360.0 * prop_variation;
                
            float radius = length(origin - center);
            vec2 dir = buildRot(prop_angle + addedRot) * vec2(-radius, 0);
            vec2 centerToUv = uv - center;

            // project originToUv along dir to get gradiant (range -1.0 to 1.0)
            float grad = distAlongDir(centerToUv, dir);

            // map to 0.0 to 1.0
            grad = grad * 0.5 + 0.5;
            //grad = grad * 2.0 - 1.0;

            return vec4(vec3(grad), 1.0);
        }
        `;

		this.buildShader(source);
	}
}
