import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";
import { Gradient } from "@/lib/designer/gradient";

// https://www.shadertoy.com/view/XljGzV
// https://www.rapidtables.com/convert/color/rgb-to-hsl.html
// https://gist.github.com/yiwenl/745bfea7f04c456e0101
export class HslExtract extends GpuDesignerNode {
	public init() {
		this.title = "HSL Extract";

		this.addInput("image");

		let prop = this.addEnumProperty("source", "Source", [
			"Hue",
			"Saturation",
			"Lightness"
		]);

		const source = `

		vec3 rgb2hsl( in vec3 c ){
			float h = 0.0;
			  float s = 0.0;
			  float l = 0.0;
			  float r = c.r;
			  float g = c.g;
			  float b = c.b;
			  float cMin = min( r, min( g, b ) );
			  float cMax = max( r, max( g, b ) );
		  
			  l = ( cMax + cMin ) / 2.0;
			  if ( cMax > cMin ) {
				  float cDelta = cMax - cMin;
				  
				  //s = l < .05 ? cDelta / ( cMax + cMin ) : cDelta / ( 2.0 - ( cMax + cMin ) ); Original
				  s = l < .0 ? cDelta / ( cMax + cMin ) : cDelta / ( 2.0 - ( cMax + cMin ) );
				  
				  if ( r == cMax ) {
					  h = ( g - b ) / cDelta;
				  } else if ( g == cMax ) {
					  h = 2.0 + ( b - r ) / cDelta;
				  } else {
					  h = 4.0 + ( r - g ) / cDelta;
				  }
		  
				  if ( h < 0.0) {
					  h += 6.0;
				  }
				  h = h / 6.0;
			  }
			  return vec3( h, s, l );
		  }

        vec4 process(vec2 uv)
        {
            // grayscale input color
            vec4 col = texture(image, uv);

			vec3 hsl = rgb2hsl(col.rgb);
			
			float result = 0.0;
			if (prop_source == 0) result = hsl.x;
			if (prop_source == 1) result = hsl.y;
			if (prop_source == 2) result = hsl.z;
            
            return vec4(vec3(result), 1.0);
        }
          `;

		this.buildShader(source);
	}
}
