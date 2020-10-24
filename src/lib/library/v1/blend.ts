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
			"Screen"
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

            // apply opacity
            col.rgb = mix(colB.rgb, col.rgb, vec3(finalOpacity));

            return col;
        }
        `;

		this.buildShader(source);
	}
}
