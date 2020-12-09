import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";
import { Gradient } from "@/lib/designer/gradient";

// https://blog.selfshadow.com/publications/blending-in-detail/
// https://blog.selfshadow.com/sandbox/normals.html
// https://www.shadertoy.com/view/4t2SzR
export class CombineNormals extends GpuDesignerNode {
	public init() {
		this.title = "Combine Normals";

		this.addInput("detail");
		this.addInput("base");

		let prop = this.addEnumProperty("technique", "Technique", [
			"Linear Blending",
			"Overlay Blending",
			"Whiteout",
			"Detail Oriented"
		]);
		prop.setValue(3);

		const source = `

		float overlay(float x, float y)
		{
			if (x < 0.5)
				return 2.0*x*y;
			else
				return 1.0 - 2.0*(1.0 - x)*(1.0 - y);
		}

        vec4 process(vec2 uv)
        {
			// sample and decode
			vec3 baseMap = texture(base,uv).xyz;
			vec3 detailMap = texture(detail,uv).xyz;
            vec3 n1 = baseMap * 2.0 - 1.0;
			vec3 n2 = detailMap * 2.0 - 1.0;
			vec3 result = vec3(0.0);
			
			// Linear Blending
			if(prop_technique == 0) {
				result = normalize(n1 + n2);
			}
			// Overlay Blending
			else if(prop_technique == 1) {
				n1 = baseMap;
				n2 = detailMap;

				result.x = overlay(n1.x, n2.x);
				result.y = overlay(n1.y, n2.y);
				result.z = overlay(n1.z, n2.z);

				result = normalize(result * 2.0 - 1.0);
			}
			// Whiteout
			else if(prop_technique == 2) {
				result = normalize(vec3(n1.xy + n2.xy, n1.z*n2.z));
			}
			// Detail Oriented
			else if(prop_technique == 3) {
				n1 = baseMap * vec3( 2,  2, 2) + vec3(-1, -1,  0);
				n2 = detailMap * vec3(-2, -2, 2) + vec3( 1,  1, -1);
				result = n1*dot(n1, n2)/n1.z - n2;
			}

			// encode
			result = result * 0.5 + 0.5;
            
            return vec4(result, 1.0);
        }
          `;

		this.buildShader(source);
	}
}
