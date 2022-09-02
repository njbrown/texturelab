import { GpuDesignerNode } from "../../designer/gpudesignernode";

//https://www.shadertoy.com/view/3dKczy
export class Curvature extends GpuDesignerNode{
    public init() {
        this.title = "Curvature";

		this.addInput("height");//height
        
        this.addEnumProperty("type", "Type", [
			"Mix curvature",
			"Sharp curvature",
			"Medium curvature",
			"Smooth curvature"
		]);

        this.addFloatProperty("intensity", "Intensity", 1, 0, 2, 0.1);
        this.addFloatProperty("angle", "Angle", 0.5, -1.0, 1, 0.1);
		this.addFloatProperty("samples", "Samples", 4, 0, 12, 1);
        
        const advancedProps = this.createGroup("Advanced properties");
        advancedProps.collapsed = false;
        advancedProps.add(
            this.addFloatProperty("sh_c", "Sharp Curvature", 0.5, 0.0, 1, 0.1)
        );
        advancedProps.add(
            this.addFloatProperty("me_c", "Mediun Curvature", 1.5, 0.0, 3, 0.1)
        );
        
        advancedProps.add(
            this.addFloatProperty("sm_c", "Smooth Curvature", 3.0, 0.0, 4, 0.1)
        );
        

		const source = `
        float HeightMap( vec2 p ){
            return texture(height, p).x;
        }
        float Curve( vec2 p, vec2 o ){
            float a = HeightMap(p+o);
            float b = HeightMap(p-o);
            return -a - b;
        }
        float CurvatureMap( vec2 p, float r ){
            float q = prop_samples; // sample quality
            float s = r/q;
            float H = HeightMap(p)*2.0;
            float v = 0.0;
            vec2 o;
            for( o.x = -q; o.x < q; o.x++ )
            for( o.y = -q; o.y < q; o.y++ ){
                float c = Curve(p, o*s);
                v += (H + c) * ((r-length(o*s)) / r);
            }
            return v/(q*q);
        }

        vec4 process(vec2 uv)
        {
            if (!height_connected)
                return vec4(0,0,0,1.0);

            vec4 color = texture(height,uv);
            float i = prop_intensity;   //Intesity

            float c;

            switch (prop_type) {
                case 0: //Mix curvature
                c += CurvatureMap(uv, i*prop_sh_c*0.01)*8.0; // sharp curvature
                c += CurvatureMap(uv, i*prop_me_c*0.01)*3.0; // medium
                c += CurvatureMap(uv, i*prop_sm_c*0.01)*1.5; // smooth curvature
                    break;
                case 1: //Sharp curvature
                c += CurvatureMap(uv, i*prop_sh_c*0.01)*8.0; // sharp curvature
                    break;
                case 2: //Medium curvature
                c += CurvatureMap(uv, i*prop_me_c*0.01)*3.0; // medium
                    break;
                case 3: //Smooth curvature
                c += CurvatureMap(uv, i*prop_sm_c*0.01)*1.5; // smooth curvature
                    break;
            }
            color.rgb = vec3( prop_angle + c );
            color.a = 1.0;

            return color;
        }
        `;

        this.buildShader(source);
    }

}