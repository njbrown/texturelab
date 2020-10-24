import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class SplatNode extends GpuDesignerNode {
	public init() {
		this.title = "Splat";

		this.addInput("image");

		this.addIntProperty("count", "Count", 50, 0, 1000, 1);

		const source = `
        // https://github.com/glslify/glsl-inverse/blob/master/index.glsl
        // mat3 inverse(mat3 m) {
        //     float a00 = m[0][0], a01 = m[0][1], a02 = m[0][2];
        //     float a10 = m[1][0], a11 = m[1][1], a12 = m[1][2];
        //     float a20 = m[2][0], a21 = m[2][1], a22 = m[2][2];
          
        //     float b01 = a22 * a11 - a12 * a21;
        //     float b11 = -a22 * a10 + a12 * a20;
        //     float b21 = a21 * a10 - a11 * a20;
          
        //     float det = a00 * b01 + a01 * b11 + a02 * b21;
          
        //     return mat3(b01, (-a22 * a01 + a02 * a21), (a12 * a01 - a02 * a11),
        //                 b11, (a22 * a00 - a02 * a20), (-a12 * a00 + a02 * a10),
        //                 b21, (-a21 * a00 + a01 * a20), (a11 * a00 - a01 * a10)) / det;
        //   }

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

        // if uv is out of bounds then return vec4(0)
        vec4 sampleImage(vec2 uv)
        {
            if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0)
                return texture(image, uv);
            return vec4(0.0);
        }

        const int MAX_ITER = 1000;

        // https://stackoverflow.com/questions/38986208/webgl-loop-index-cannot-be-compared-with-non-constant-expression
        vec4 process(vec2 uv)
        {
            vec4 color = vec4(0.0);
            for(int i = 0; i<MAX_ITER; i++)
            {
                if (i >= prop_count)
                    break;

                float x = randomFloatRange(i*10 + 1, -0.5, 0.5);
                float y = randomFloatRange(i*13 + 2, -0.5, 0.5);
                float r = randomFloatRange(i*15 + 3, 0.0, 360.0);

                vec2 sampleUV = transformUV(uv, vec2(x,y), r, vec2(1.0));
                color += sampleImage(sampleUV);

                //sample 4 sides
                sampleUV = transformUV(uv, vec2(x,y) + vec2(-1.0,  0.0), r, vec2(1.0));
                color += sampleImage(sampleUV);
                sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0,  0.0), r, vec2(1.0));
                color += sampleImage(sampleUV);
                sampleUV = transformUV(uv, vec2(x,y) + vec2( 0.0,  1.0), r, vec2(1.0));
                color += sampleImage(sampleUV);
                sampleUV = transformUV(uv, vec2(x,y) + vec2( 0.0, -1.0), r, vec2(1.0));
                color += sampleImage(sampleUV); 
            }

            //color = color / vec4(float(prop_count));

            return color;
        }
        `;

		this.buildShader(source);
	}
}
