import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class WarpNodeV2 extends GpuDesignerNode {
	public init() {
		this.title = "Warp";

		this.addInput("inputImage");
		this.addInput("height");

		this.addFloatProperty("intensity", "Intensity", 0.1, -1.0, 1.0, 0.01);

		// calculates normal, then warps uv by it
		const source = `
        vec2 calcSlope(vec2 uv)
        {
            vec3 sl = vec3(0.0,0.0,0.0);
            sl.x = texture(height, uv + vec2(0.0, 1.0/_textureSize.y)).r;
            sl.y = texture(height, uv + vec2(-1.0/_textureSize.x, -0.5/_textureSize.y)).r;
            sl.z = texture(height, uv + vec2(1.0/_textureSize.x, -0.5/_textureSize.y)).r;
            
            vec2 result = vec2(0.0);
            result.x = sl.z-sl.y;
            result.y = dot(sl, vec3(1, -0.5, -0.5));
            
            return result;
        }

        vec4 process(vec2 uv)
        {
            vec2 step = vec2(1.0,1.0)/_textureSize;
            vec4 warpCol = texture(height, uv);
            vec2 warp = calcSlope(uv);

            vec4 color = texture(inputImage, uv + warp * prop_intensity);

            return color;
        }
        `;

		this.buildShader(source);
	}
}
