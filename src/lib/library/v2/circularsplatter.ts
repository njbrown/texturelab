import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class CircularSplatter extends GpuDesignerNode {
	public init() {
		this.title = "Circular Splatter";

		this.addInput("image");
		this.addInput("mask");
		this.addInput("size");
		this.addInput("intensity");

        this.addFloatProperty("radius", "Radius", 0.5, 0, 1.0, 0.01);

		this.addIntProperty("count", "Count", 10, 0, 50, 1);
        this.addIntProperty("rings", "Rings", 1, 0, 5, 1);
        
        this.addFloatProperty("rot", "Rotation", 0, 0, 360, 0.1);

        this.addFloatProperty("intensityRand", "Random Intensity", 0, 0, 1.0, 0.01);

        this.addFloatProperty("scale", "Scale", 0.1, 0, 4, 0.1);
		this.addFloatProperty("scaleRand", "Scale random", 0, 0, 1, 0.1);

        this.addEnumProperty("blendType", "Blend Type", [
			"Max",
			"Add"
        ]);
        
		const source = `
        mat3 transMat(vec2 t)
        {
            return mat3(vec3(1.0,0.0,0.0), vec3(0.0,1.0,0.0), vec3(t, 1.0));
        }

        mat3 scaleMat(vec2 s)
        {
            return mat3(vec3(s.x,0.0,0.0), vec3(0.0,s.y,0.0), vec3(0.0, 0.0, 1.0));
        }

        mat3 rotMat(float rot)
        {
            float r = radians(rot);
            return mat3(vec3(cos(r), -sin(r),0.0), vec3(sin(r), cos(r),0.0), vec3(0.0, 0.0, 1.0));
        }

        vec2 transformUV(vec2 uv, vec2 translate, float rot, vec2 scale)
        {
            mat3 trans = transMat(vec2(0.5, 0.5)) *
                transMat(vec2(translate.x, translate.y)) *
                rotMat(rot) *
                scaleMat(vec2(scale.x, scale.y)) *
                transMat(vec2(-0.5, -0.5));

            vec3 res = inverse(trans) * vec3(uv, 1.0);
            uv = res.xy;
            
            return clamp(uv,vec2(0.0), vec2(1.0));
        }

        float randomFloatRange(int index, float fmin, float fmax)
        {
            float r = _rand(vec2(_seed) + vec2(float(index) * 0.0001));

            return fmin + (fmax - fmin) * r;
        }

        float randomFloatRange(vec2 seed, float fmin, float fmax)
        {
            float r = _rand(vec2(_seed) + vec2(seed * 0.0001));

            return fmin + (fmax - fmin) * r;
        }

        // if uv is out of bounds then return vec4(0)
        vec4 sampleImage(sampler2D image, vec2 uv)
        {
            if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0)
                return texture(image, uv);
            return vec4(0.0);
        }

        float calcIntensity(int i, vec2 uv)
		{
			// intensity
			float intens = 1.0;
			// multiply by texture if available
			if (intensity_connected) {
				intens *= texture(intensity, uv).r;
			}
			// random intensity per tile
			float randIntensity = randomFloatRange(uv, 0.0, 1.0);

			// lerp between random intensity and one by image ( or 1.0 )
			intens = mix(intens, randIntensity, prop_intensityRand);

			return intens;
        }
        
        vec2 calcScale(int i, vec2 uv)
		{
			float s = prop_scale;
			if (size_connected) {
				s *= texture(size, uv).r;
			}

            //float randScale = randomFloatRange(i*11 + 6, 0.0, 1.0);
            float randScale = randomFloatRange(uv, 0.0, 1.0);
            
			s = mix(s, randScale, prop_scaleRand);

			return vec2(s);
		}

        vec4 blend(vec4 colA, vec4 colB)
        {
            vec4 col = vec4(1.0);
            if (prop_blendType==0) // max
                col.rgb = max(colA.rgb, colB.rgb);
            if (prop_blendType==1) // add
                col.rgb = colA.rgb + colB.rgb;

            return col;
        }

        // https://stackoverflow.com/questions/38986208/webgl-loop-index-cannot-be-compared-with-non-constant-expression
        vec4 process(vec2 uv)
        {
            vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
            float spacing = prop_radius/float(prop_rings);
            float angle_spacing = 360.0/float(prop_count);

            // calculate rings from the outside going in
            for(int ring = 0; ring<prop_rings; ring++)
            {
                float radius = prop_radius - spacing * float(ring);
                for(int i = 0; i<prop_count; i++)
                {
                    float angle = angle_spacing * float(i);

                    float x = cos(radians(angle)) * radius;
                    float y = sin(radians(angle)) * radius;
                    float r = angle;
                    // float s = 1.0;

                    //vec2 center = transformUV(vec2(0.5, 0.5), vec2(x,y), r, vec2(1.0));

                    if (mask_connected) {
                        float mask = texture(mask, vec2(x,y) + vec2(0.5)).r;
                        if (mask < 0.001f)
                            continue;
                    }

                    // if (size_connected) {
                    //     s = s * texture(size, vec2(x,y) + vec2(0.5)).r;
                    // }

                    vec2 s = calcScale(i * ring, vec2(x,y) + vec2(0.5));

                    float intens = calcIntensity(i * ring, vec2(x,y) + vec2(0.5));

                    vec2 sampleUV = transformUV(uv, vec2(x,y), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);

                    // sample 4 sides
                    sampleUV = transformUV(uv, vec2(x,y) + vec2(-1.0,  0.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0,  0.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 0.0,  1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 0.0, -1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);

                    // sample 4 diagonal sides
                    sampleUV = transformUV(uv, vec2(x,y) + vec2(-1.0,  1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0,  1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0,  1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0, -1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                }
            }

            //color = color / vec4(float(prop_count));

            return color;
        }
        `;

		this.buildShader(source);
	}
}
