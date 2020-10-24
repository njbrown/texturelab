import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";

export class GradientNode extends GpuDesignerNode {
	public init() {
		this.title = "Gradient";

		const white = new Color();
		white.r = 1;
		white.g = 1;
		white.b = 1;

		this.addColorProperty("colorA", "Color A", Color.parse("#000000"));
		this.addFloatProperty("posA", "Position A", 0, 0, 1, 0.01);
		this.addColorProperty("colorB", "Color B", white);
		this.addFloatProperty("posB", "Position B", 1, 0, 1, 0.01);

		this.addEnumProperty("mode", "Gradient Direction", [
			"Left To Right",
			"Right To Left",
			"Top To Bottom",
			"Bottom To Top"
		]);

		const source = `
        #define POINTS_MAX 32

        // assumes points are sorted
        vec3 calcGradient(float t, vec3 colors[POINTS_MAX], float positions[POINTS_MAX], int numPoints)
        {
            if (numPoints == 0)
                return vec3(1,0,0);
            
            if (numPoints == 1)
                return colors[0];
            
            // here at least two points are available
            if (t < positions[0])
                return colors[0];
            
            int last = numPoints - 1;
            if (t > positions[last])
                return colors[last];
            
            // find two points in-between and lerp
            
            for(int i = 0; i < numPoints-1;i++) {
                if (positions[i+1] > t) {
                    vec3 colorA = colors[i];
                    vec3 colorB = colors[i+1];
                    
                    float t1 = positions[i];
                    float t2 = positions[i+1];
                    
                    float lerpPos = (t - t1)/(t2 - t1);
                    return mix(colorA, colorB, lerpPos);
                    
                }
                
            }
            
            return vec3(0,0,0);
        }


        vec4 process(vec2 uv)
        {
            float t = 0.0;
    
            // left to right
            if (prop_mode == 0)
                t = uv.x;
            // right to left
            else if (prop_mode == 1)
                t = 1.0 - uv.x;
            // top to bottom
            else if (prop_mode == 2)
                t = 1.0 - uv.y;
            // bottom to top
            else if (prop_mode == 3)
                t = uv.y;


            vec3 colors[POINTS_MAX];
            colors[0] = prop_colorA.rgb;
            colors[1] = prop_colorB.rgb;
            float positions[POINTS_MAX];
            positions[0] = prop_posA;
            positions[1] = prop_posB;
                
            
            vec3 col = calcGradient(t, colors, positions, 2);
            
            return vec4(col,1.0);
        }
        `;

		this.buildShader(source);
	}
}

export class TriGradientNode extends GpuDesignerNode {
	public init() {
		this.title = "TriGradient";

		const white = new Color();
		white.r = 1;
		white.g = 1;
		white.b = 1;

		this.addColorProperty("colorA", "Color A", Color.parse("#000000"));
		this.addFloatProperty("posA", "Position A", 0, 0, 1, 0.01);

		this.addColorProperty("colorB", "Color B", white);
		this.addFloatProperty("posB", "Position B", 0.5, 0, 1, 0.01);

		this.addColorProperty("colorC", "Color C", Color.parse("#000000"));
		this.addFloatProperty("posC", "Position C", 1, 0, 1, 0.01);

		this.addEnumProperty("mode", "Gradient Direction", [
			"Left To Right",
			"Right To Left",
			"Top To Bottom",
			"Bottom To Top"
		]);

		const source = `
          #define POINTS_MAX 32
  
          // assumes points are sorted
          vec3 calcGradient(float t, vec3 colors[POINTS_MAX], float positions[POINTS_MAX], int numPoints)
          {
              if (numPoints == 0)
                  return vec3(1,0,0);
              
              if (numPoints == 1)
                  return colors[0];
              
              // here at least two points are available
              if (t < positions[0])
                  return colors[0];
              
              int last = numPoints - 1;
              if (t > positions[last])
                  return colors[last];
              
              // find two points in-between and lerp
              
              for(int i = 0; i < numPoints-1;i++) {
                  if (positions[i+1] > t) {
                      vec3 colorA = colors[i];
                      vec3 colorB = colors[i+1];
                      
                      float t1 = positions[i];
                      float t2 = positions[i+1];
                      
                      float lerpPos = (t - t1)/(t2 - t1);
                      return mix(colorA, colorB, lerpPos);
                      
                  }
                  
              }
              
              return vec3(0,0,0);
          }
  
  
          vec4 process(vec2 uv)
          {
              float t = 0.0;
      
              // left to right
              if (prop_mode == 0)
                  t = uv.x;
              // right to left
              else if (prop_mode == 1)
                  t = 1.0 - uv.x;
              // top to bottom
              else if (prop_mode == 2)
                  t = 1.0 - uv.y;
              // bottom to top
              else if (prop_mode == 3)
                  t = uv.y;
  
  
              vec3 colors[POINTS_MAX];
              colors[0] = prop_colorA.rgb;
              colors[1] = prop_colorB.rgb;
              colors[2] = prop_colorC.rgb;
              float positions[POINTS_MAX];
              positions[0] = prop_posA;
              positions[1] = prop_posB;
              positions[2] = prop_posC;
                  
              
              vec3 col = calcGradient(t, colors, positions, 3);
              
              return vec4(col,1.0);
          }
          `;

		this.buildShader(source);
	}
}
