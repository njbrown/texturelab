import { GpuDesignerNode } from "../../designer/gpudesignernode";

// uses single pass gaussian
// https://www.shadertoy.com/view/4tSyzy
// https://stackoverflow.com/questions/2157920/why-define-pi-4atan1-d0
export class BlurV2 extends GpuDesignerNode {
	public init() {
		this.title = "Blur";

		this.addInput("image");

		this.addFloatProperty("intensity", "Intensity", 1, 0, 10, 0.1);
		this.addIntProperty("samples", "Samples", 50, 0, 100, 1);

		const source = `
        #define pow2(x) (x * x)

        const float pi = atan(1.0) * 4.0;

        float gaussian(vec2 i, float sigma) {
            return 1.0 / (2.0 * pi * pow2(sigma)) * exp(-((pow2(i.x) + pow2(i.y)) / (2.0 * pow2(sigma))));
        }

        vec3 blur(sampler2D sp, vec2 uv, vec2 scale) {
            vec3 col = vec3(0.0);
            float accum = 0.0;
            float weight;
            vec2 offset;

            float sigma = float(prop_samples) * 0.25;
            
            for (int x = -prop_samples / 2; x < prop_samples / 2; ++x) {
                for (int y = -prop_samples / 2; y < prop_samples / 2; ++y) {
                    offset = vec2(x, y);
                    weight = gaussian(offset, sigma);
                    col += texture(sp, uv + scale * offset).rgb * weight;
                    accum += weight;
                }
            }
            
            return col / accum;
        }

        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0,0,0,1.0);

            vec4 color = vec4(0.0);
            vec2 ps = vec2(1.0, 1.0) / _textureSize;
            color.rgb = blur(image, uv, ps * prop_intensity);
            color.a = 1.0;

            return color;
        }
        `;

		this.buildShader(source);
	}
}
