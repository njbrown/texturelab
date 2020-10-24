import { GpuDesignerNode } from "../../designer/gpudesignernode";

// https://thebookofshaders.com/edit.php#12/tissue.frag
export class LineCellNode extends GpuDesignerNode {
	public init() {
		this.title = "Lined Cell";

		this.addIntProperty("scale", "Scale", 5, 0, 256);
		this.addBoolProperty("invert", "Invert", false);
		this.addFloatProperty("entropy", "Entropy", 0, 0, 1, 0.01);
		this.addFloatProperty("intensity", "Intensity", 1, 0, 2, 0.01);
		this.addFloatProperty("thickness", "Line Thickness", 0.1, 0, 0.2, 0.01);

		const source = `
        vec2 random2( vec2 p ) {
            return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
        }

        vec3 voronoi(vec2 uv)
        {
            vec2 i_st = floor(uv);
            vec2 f_st = fract(uv);

            float m_dist = 1.;
            vec2 mg, mr;
            float md = 8.0;
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

                    // Vector between the pixel and the point
                    vec2 diff = neighbor + point - f_st;

                    // Distance to the point
                    float dist = length(diff);

                    // Keep the closer distance
                    //m_dist = min(m_dist, dist);
                    if (dist < md) {
                        closestPoint = neighborCell;
                        md = dist;
                        mr = diff;
                        mg = neighbor;
                    }
                }
            }

            // if both points are closest to each other then
            // find the line inbetween
            // this means that we wrap the neighbor the same way
            // as we did above
            md = 8.0;
            for (int j=-2; j<=2; j++ ) {
                for (int i=-2; i<=2; i++ ) {
                    vec2 neighbor = mg + vec2(float(i),float(j)); // actual pos
                    vec2 neighborCell = mod(i_st + neighbor, float(prop_scale)); // wrapped pos

                    ////vec2 o = random2(i_st + neighbor);
                    vec2 point = random2(neighborCell);
                    
                    point = mix(point, vec2(0.5,0.5), prop_entropy);

                    //vec2 r = g + o - f_st;
                    vec2 diff = neighbor + point - f_st;

                    if( dot(mr-diff,mr-diff)>0.00001 )// if they're both their nearest
                        md = min( md, dot( 0.5*(mr+diff), normalize(diff-mr) ) );
                }
            }

            return vec3(md, mr);
        }

        vec4 process(vec2 uv)
        {
            uv *= float(prop_scale);
            vec3 c = voronoi(uv);

            vec3 color = vec3(0.0);

            color = mix( vec3(1.0), color, smoothstep( prop_thickness, prop_thickness, c.x ) );

            if (prop_invert)
                color = vec3(1.0) - color;

            return vec4(color * prop_intensity, 1.0);
        }
        `;

		this.buildShader(source);
	}
}
