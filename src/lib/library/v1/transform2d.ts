import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class Transform2DNode extends GpuDesignerNode {
	public init() {
		this.title = "Transform2D";

		this.addInput("image");

		this.addFloatProperty("translateX", "Translate X", 0, -1.0, 1.0, 0.01);
		this.addFloatProperty("translateY", "Translate Y", 0, -1.0, 1.0, 0.01);

		this.addFloatProperty("scaleX", "Scale X", 1, -2.0, 2.0, 0.01);
		this.addFloatProperty("scaleY", "Scale Y", 1, -2.0, 2.0, 0.01);

		this.addFloatProperty("rot", "Rotation", 0, 0.0, 360.0, 0.01);

		this.addBoolProperty("clamp", "Clamp", true);

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

        mat2 buildScale(float sx, float sy)
        {
            return mat2(sx, 0.0, 0.0, sy);
        }

        // rot is in degrees
        mat2 buildRot(float rot)
        {
            float r = radians(rot);
            return mat2(cos(r), -sin(r), sin(r), cos(r));
        }
        
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

        vec4 process(vec2 uv)
        {
            // transform by (-0.5, -0.5)
            // scale
            // rotate
            // transform
            // transform by (0.5, 0.5)  

            mat3 trans = transMat(vec2(0.5, 0.5)) *
                transMat(vec2(prop_translateX, prop_translateY)) *
                rotMat(prop_rot) *
                scaleMat(vec2(prop_scaleX, prop_scaleY)) *
                transMat(vec2(-0.5, -0.5));

            vec3 res = inverse(trans) * vec3(uv, 1.0);
            uv = res.xy;


            if (prop_clamp)
                return texture(image, clamp(uv,vec2(0.0), vec2(1.0)));
            return texture(image, uv);
        }
        `;

		this.buildShader(source);
	}
}
