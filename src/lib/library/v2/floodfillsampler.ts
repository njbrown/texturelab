import { Color } from "@/lib/designer/color";
import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class FloodFillSampler extends GpuDesignerNode {
	public init() {
		this.title = "Flood Fill Sampler";

		this.addInput("floodfill");
		this.addInput("image");
		this.addInput("mask");
		this.addInput("size");
		this.addInput("intensity");

		this.addFloatProperty("rot", "Rotation", 0, 0, 360, 0.1);
		this.addFloatProperty("rotRand", "Random Rotation", 0, 0, 1.0, 0.01);
		this.addFloatProperty("posRand", "Random Position", 0, 0, 1.0, 0.01);
		this.addFloatProperty("intensityRand", "Random Intensity", 0, 0, 1.0, 0.01);
		this.addFloatProperty("scale", "Scale", 1, 0, 4, 0.1);
		this.addFloatProperty("scaleRand", "Scale random", 0, 0, 1, 0.1);
		
		this.addIntProperty("precision", "Precision", 2, 1, 3, 1);

		this.addColorProperty("bg", "Background Color", new Color());

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

        float randomFloatRange(vec2 center, int offset, float fmin, float fmax)
        {
            float r = _rand(vec2(_seed) + center + vec2(float(offset)) * 0.01);

            return fmin + (fmax - fmin) * r;
        }

		// if uv is out of bounds then return vec4(0)
        vec4 sampleImage(sampler2D image, vec2 uv)
        {
            if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0)
                return texture(image, uv);
            return vec4(prop_bg.rgb, 1.0);
		}

		float calcIntensity(vec2 center)
		{
			// intensity
			float intens = 1.0;
			// multiply by texture if available
			if (intensity_connected) {
				intens *= texture(intensity, center).r;
			}
			// random intensity per tile
			float randIntensity = randomFloatRange(center, 8, 0.0, 1.0);

			// lerp between random intensity and one by image ( or 1.0 )
			intens = mix(intens, randIntensity, prop_intensityRand);

			return intens;
		}

		vec2 calcRandomOffset(vec2 center)
		{
			float randPosX = randomFloatRange(center, 4, -1.0, 1.0);
			float randPosY = randomFloatRange(center, 5, -1.0, 1.0);
			vec2 randPos = normalize(vec2(randPosX, randPosY)) * prop_posRand;
			return vec2(randPos.x, randPos.y);
		}

		vec2 calcScale(vec2 center)
		{
			float s = prop_scale;
			if (size_connected) {
				s *= texture(size, center).r;
			}

			float randScale = randomFloatRange(center, 3, 0.0, 1.0);
			// lerp between random scale and one by image ( or 1.0 )
			s = mix(s, randScale, prop_scaleRand);

			return vec2(s);
		}

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
            vec4 pixelData = texture(floodfill, uv);
            if (pixelData.ba == vec2(0.0, 0.0))
				return vec4(prop_bg.rgb, 1.0);
				
			vec2 center = calcFloodFillCenter(uv, pixelData);

			// convert to local
            center.x = wrapAround(center.x, 1.0);
            center.y = wrapAround(center.y, 1.0);

			// quantize center to remove minor innaccuracies
            // the hash function is very sensitive to even small changes
            int place = int(pow(float(10), float(prop_precision)));
            center.x = floatRound(center.x, place);
            center.y = floatRound(center.y, place);
				
			vec2 t = calcRandomOffset(center);
			float intens = calcIntensity(center);
			float r = randomFloatRange(center, 3, -180.0, 180.0) * prop_rotRand + prop_rot;
			
			if (mask_connected) {
				float mask = texture(mask, center).r;
				if (mask < 0.001f)
					return vec4(prop_bg.rgb, 1.0);
			}

			vec2 s = calcScale(center);

			vec2 finalUv = transformUV(pixelData.rg, t, r, s);
            vec3 color = texture(image, finalUv).rgb;

            return vec4(color * intens, 1.0);
        }
        `;

		this.buildShader(source);
	}
}
