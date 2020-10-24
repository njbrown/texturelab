import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class SolidCellNode extends GpuDesignerNode {
	public init() {
		this.title = "Solid Cell";

		this.addIntProperty("scale", "Scale", 5, 0, 256);
		this.addBoolProperty("invert", "Invert", false);
		this.addFloatProperty("entropy", "Entropy", 0, 0, 1, 0.01);
		this.addFloatProperty("intensity", "Intensity", 1, 0, 2, 0.01);

		const source = `
        vec2 random2( vec2 p ) {
            return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
        }

        vec4 process(vec2 uv)
        {
            uv *= float(prop_scale);

            vec2 i_st = floor(uv);
            vec2 f_st = fract(uv);

            float m_dist = 1.;
            vec2 closestPoint = vec2(0.5,0.5);

            for (int y= -1; y <= 1; y++) {
                for (int x= -1; x <= 1; x++) {
                    vec2 neighbor = vec2(float(x),float(y));
                    // wraps around cells to make it seamless
                    vec2 neighborCell = mod(i_st + neighbor, float(prop_scale));

                    // Random position from current + neighbor place in the grid
                    vec2 point = random2(neighborCell);

                    // entropy is lerping between the center and the random point
                    point = mix(point, vec2(0.5,0.5), prop_entropy);

                    // Animate the point
                    //point = 0.5 + 0.5*sin(u_time + 6.2831*point);

                    // Vector between the pixel and the point
                    vec2 diff = neighbor + point - f_st;

                    // Distance to the point
                    float dist = length(diff);

                    // Keep the closer distance
                    //m_dist = min(m_dist, dist);
                    if (dist < m_dist) {
                        m_dist = dist;
                        closestPoint = neighborCell;
                    }
                }
            }

            vec3 color = vec3(_rand(vec2(_seed)+closestPoint));
            if (prop_invert)
                color = vec3(1.0) - color;

            return vec4(color * prop_intensity, 1.0);
        }
        `;

		this.buildShader(source);
	}
}
