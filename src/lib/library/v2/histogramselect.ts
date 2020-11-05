import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class HistogramSelect extends GpuDesignerNode {
	public init() {
		this.title = "Histogram Select";

		this.addInput("image");

		this.addFloatProperty("position", "Position", 0.5, 0.0, 1.0, 0.01);
		this.addFloatProperty("range", "Range", 0.1, 0.0, 1.0, 0.01);
		this.addFloatProperty("contrast", "Contrast", 0.01, 0.01, 1.0, 0.01);
		this.addBoolProperty("invert", "Invert", false);

		const source = `
        vec4 process(vec2 uv)
        {
            vec4 a =  texture(image, uv);

            vec3 col = a.rgb;
    
            vec3 diff = abs(col - vec3(prop_position));
            diff -= vec3(prop_range);
            diff = clamp(diff, 0.0, prop_contrast);
            
            vec3 result = diff * (1.0 / prop_contrast);

            if(prop_invert)
                return vec4(result, a.a);
            else
                return vec4(vec3(1.0) - result, a.a);
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

//     vec3 col = vec3(uv.x);

//     float contrast = 0.01;
//     float position = 0.7;
//     vec3 diff = abs(col - vec3(position));
//     diff = clamp(diff, 0.0, contrast);

//     vec3 result = diff * (1.0 / contrast);

//     // Output to screen
//     //fragColor = vec4(vec3( 1.0 - result),1.0);
//     //fragColor = vec4(vec3(result),1.0);
//     fragColor = vec4(vec3(1.0) - result,1.0);
// }
