import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class BlendNode extends GpuDesignerNode {
	public init() {
		this.title = "Blend";

		this.addInput("colorA"); // foreground
		this.addInput("colorB"); // background
		this.addInput("opacity");

		this.addEnumProperty("type", "Type", [
			"Multiply",
			"Add",
			"Subtract",
			"Divide",
			//   "Add Sub",
			"Max",
			"Min",
			"Switch",
			"Overlay",
			"Screen",
            //  News blend mode
            "Exclusion",
            "Hard light",
            "Soft light",
            "Color dodge",
            "Color burn",
            "Linear burn"
		]);
		this.addFloatProperty("opacity", "Opacity", 1.0, 0.0, 1.0, 0.01);

		const source = `

        float screen(float fg, float bg) {
            float res = (1.0 - fg) * (1.0 - bg);
            return 1.0 - res;
        }
        vec4 process(vec2 uv)
        {
            float finalOpacity = prop_opacity;
            if (opacity_connected)
                finalOpacity *= texture(opacity, uv).r;

            vec4 colA = texture(colorA,uv);
            vec4 colB = texture(colorB,uv);
            vec4 col = vec4(1.0);

            if (prop_type==0){ // multiply
                col.rgb = colA.rgb * colB.rgb;
            }
            if (prop_type==1) // add
                col.rgb = colA.rgb + colB.rgb;
            if (prop_type==2) // subtract
                col.rgb = colB.rgb - colA.rgb;
            if (prop_type==3) // divide
                col.rgb = colB.rgb / colA.rgb;
            // if (prop_type==4) {// add sub
            //     if (colA.r > 0.5) col.r = colB.r + colA.r; else col.r = colB.r - colA.r;
            //     if (colA.g > 0.5) col.g = colB.g + colA.g; else col.g = colB.g - colA.g;
            //     if (colA.b > 0.5) col.b = colB.b + colA.b; else col.b = colB.b - colA.b;
            // }
            if (prop_type==4) { // max
                col.rgb = max(colA.rgb, colB.rgb);
            }
            if (prop_type==5) { // min
                col.rgb = min(colA.rgb, colB.rgb);
            }
            if (prop_type==6) { // switch
                col.rgb = colA.rgb;
            }
            if (prop_type==7) { // overlay
                if (colB.r < 0.5) col.r = colB.r * colA.r; else col.r = screen(colB.r, colA.r);
                if (colB.g < 0.5) col.g = colB.g * colA.g; else col.g = screen(colB.g, colA.g);
                if (colB.b < 0.5) col.b = colB.b * colA.b; else col.b = screen(colB.b, colA.b);
            }
            if (prop_type==8) { // screen
                col.r = screen(colA.r, colB.r);
                col.g = screen(colA.g, colB.g);
                col.b = screen(colA.b, colB.b);
            }

            if (prop_type==9){ // exclusion
                col.rgb = colB.rgb + colA.rgb - 2.0 * colB.rgb * colA.rgb;
            }
            // a = colB; b = colA
            if (prop_type==10){ // Hard light
                if(colB.r < 0.5) col.r = (2.0 * colB.r * colA.r); else col.r = (1.0 - 2.0 * (1.0 - colB.r) * (1.0 - colA.r));
                if(colB.g < 0.5) col.g = (2.0 * colB.g * colA.g); else col.g = (1.0 - 2.0 * (1.0 - colB.g) * (1.0 - colA.g));
                if(colB.b < 0.5) col.b = (2.0 * colB.b * colA.b); else col.b = (1.0 - 2.0 * (1.0 - colB.b) * (1.0 - colA.b));
                //if b < 0.5 ?           (2.0 * a * b) : (1.0 - 2.0 * (1.0 - a) * (1.0 - b))
            }
 
            if (prop_type==11){ // Soft light
                if(colB.r < 0.5) col.r = 2.0 * colB.r * colA.r + colB.r * colB.r * (1.0 - 2.0 * colA.r); else col.r = sqrt(colB.r) * (2.0 * colA.r - 1.0) + (2.0 * colB.r) * (1.0 - colA.r);
                if(colB.g < 0.5) col.g = 2.0 * colB.g * colA.g + colB.g * colB.g * (1.0 - 2.0 * colA.g); else col.g = sqrt(colB.g) * (2.0 * colA.g - 1.0) + (2.0 * colB.g) * (1.0 - colA.g);
                if(colB.b < 0.5) col.b = 2.0 * colB.b * colA.b + colB.b * colB.b * (1.0 - 2.0 * colA.b); else col.b = sqrt(colB.b) * (2.0 * colA.b - 1.0) + (2.0 * colB.b) * (1.0 - colA.b);
                //b < 0.5 ?             (2.0 * a * b + a * a * (1.0 - 2.0 * b))                          :            (sqrt(a) * (2.0 * b - 1.0) + (2.0 * a) * (1.0 - b)) 
            }
            
            if (prop_type==12){ // Color dodge
                col.rgb = colB.rgb / (1.0 - colA.rgb);
            }
            
            if (prop_type==13){ // Color burn
                col.rgb = 1.0 - (1.0 - colB.rgb) / colA.rgb;
            }
            
            if (prop_type==14){ // Linear burn
                col.rgb = colB.rgb + colA.rgb - 1.0;
            }
            


            // apply opacity
            col.rgb = mix(colB.rgb, col.rgb, vec3(finalOpacity));

            return col;
        }
        `;

		this.buildShader(source);
	}
}
