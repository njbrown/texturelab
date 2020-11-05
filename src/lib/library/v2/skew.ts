import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class Skew extends GpuDesignerNode {
	public init() {
		this.title = "Skew";

		this.addInput("image");

		this.addEnumProperty("axis", "Axis", ["Horizontal", "Vertical"]);
		this.addFloatProperty("skew", "skew", 0.5, -1.0, 1.0, 0.01);
		this.addFloatProperty("position", "Position", 0.5, 0.0, 1.0, 0.01);

		const source = `
        vec4 process(vec2 uv)
        {
            float xSkew = 0.0;
            float ySkew = 0.0;
            vec2 offset = vec2(0.0, 0.0);
            if (prop_axis == 0) {
                offset = vec2(0.0, prop_position);
                xSkew = prop_skew;
            }
            else {
                offset = vec2(prop_position, 0.0);
                ySkew = prop_skew;
            }

            mat3 skewMat = mat3(1.0, ySkew, 0.0,
                                xSkew, 1.0, 0.0,
                                0.0, 0.0, 1.0);
            
            uv -= offset;
            uv = (skewMat * vec3(uv, 1.0)).xy;
            uv += offset;

            // Time varying pixel color
            vec4 col =  texture(image, uv);
            return col;
        }
        `;

		this.buildShader(source);
	}
}

// SHADERTOY

// void mainImage( out vec4 fragColor, in vec2 fragCoord )
// {
//     // Normalized pixel coordinates (from 0 to 1)
//     vec2 uv = fragCoord/iResolution.xy;

//     float xSkew = mod(iTime, 2.0);
//     float ySkew = 0.0;//mod(iTime, 2.0);
//     mat3 skewMat = mat3(1.0, ySkew, 0.0,
//                        	xSkew, 1.0, 0.0,
//                        	0.0, 0.0, 1.0);

//     uv -= vec2(0.0, 0.5);
//     uv = (skewMat * vec3(uv, 1.0)).xy;
//     uv += vec2(0.0, 0.5);

//     // Time varying pixel color
//     vec3 col = texture(iChannel0, uv).rgb;

//     // Output to screen
//     fragColor = vec4(col,1.0);
// }
