import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class TestGradientNode extends GpuDesignerNode {
	public init() {
		this.title = "Gradient Node";
		console.log("goodbye from inside gradient node");

		const source = `
        vec4 process(vec2 uv)
        {
            return vec4(uv.x, 0, uv.y, 1);
        }
        `;

		this.buildShader(source);
	}
}

export class TestInvertNode extends GpuDesignerNode {
	public init() {
		this.title = "Invert";

		this.addInput("color");

		const source = `
        vec4 process(vec2 uv)
        {
            vec4 col = vec4(1.0) - texture(color,uv);
            col.a = 1.0;
            return col;
        }
        `;

		this.buildShader(source);
	}
}

export class TestMultiplyNode extends GpuDesignerNode {
	public init() {
		this.title = "Multiply";

		this.addInput("colorA");
		this.addInput("colorB");

		const source = `
        vec4 process(vec2 uv)
        {
            vec4 colA = texture(colorA,uv);
            vec4 colB = texture(colorB,uv);
            return colA * colB;
        }
        `;

		this.buildShader(source);
	}
}

export class TestShapeNode extends GpuDesignerNode {
	public init() {
		this.title = "Shape";
		//this.exportName = "result";

		const source = `
        #define PI 3.14159265359
        #define TWO_PI 6.28318530718

        vec4 process(vec2 uv)
        {
            vec3 color = vec3(0.0);
            float d = 0.0;

            float scale = 0.5;
            vec2 st = (uv - vec2(0.5,0.5));
            int N = 8;

            // Angle and radius from the current pixel
            float a = atan(st.x,st.y)+PI;
            float r = TWO_PI/float(N);

            // Shaping function that modulate the distance
            d = cos(floor(.9+a/r)*r-a)*length(st) * (1.0 / scale);

            color = vec3(1.0-smoothstep(.5,.500001,d));

            return vec4(color,1.0);
        }
        `;

		this.buildShader(source);
	}
}

export class TestWorleyNode extends GpuDesignerNode {
	public init() {
		this.title = "Worley";
		//this.exportName = "result";

		this.addFloatProperty("scale", "Cell Scale", 5, 1, 20);

		const source = `
        vec3 mod289(vec3 x) {
            return x - floor(x * (1.0 / 289.0)) * 289.0;
          }
          
          vec2 mod289(vec2 x) {
            return x - floor(x * (1.0 / 289.0)) * 289.0;
          }
          
          // Modulo 7 without a division
          vec3 mod7(vec3 x) {
            return x - floor(x * (1.0 / 7.0)) * 7.0;
          }
          
          // Permutation polynomial: (34x^2 + x) mod 289
          vec3 permute(vec3 x) {
            return mod289((34.0 * x + 1.0) * x);
          }
          
          // Cellular noise, returning F1 and F2 in a vec2.
          // Standard 3x3 search window for good F1 and F2 values
          vec2 cellular(vec2 P) {
          #define K 0.142857142857 // 1/7
          #define Ko 0.428571428571 // 3/7
          #define jitter 1.0 // Less gives more regular pattern
              vec2 Pi = mod289(floor(P));
               vec2 Pf = fract(P);
              vec3 oi = vec3(-1.0, 0.0, 1.0);
              vec3 of = vec3(-0.5, 0.5, 1.5);
              vec3 px = permute(Pi.x + oi);
              vec3 p = permute(px.x + Pi.y + oi); // p11, p12, p13
              vec3 ox = fract(p*K) - Ko;
              vec3 oy = mod7(floor(p*K))*K - Ko;
              vec3 dx = Pf.x + 0.5 + jitter*ox;
              vec3 dy = Pf.y - of + jitter*oy;
              vec3 d1 = dx * dx + dy * dy; // d11, d12 and d13, squared
              p = permute(px.y + Pi.y + oi); // p21, p22, p23
              ox = fract(p*K) - Ko;
              oy = mod7(floor(p*K))*K - Ko;
              dx = Pf.x - 0.5 + jitter*ox;
              dy = Pf.y - of + jitter*oy;
              vec3 d2 = dx * dx + dy * dy; // d21, d22 and d23, squared
              p = permute(px.z + Pi.y + oi); // p31, p32, p33
              ox = fract(p*K) - Ko;
              oy = mod7(floor(p*K))*K - Ko;
              dx = Pf.x - 1.5 + jitter*ox;
              dy = Pf.y - of + jitter*oy;
              vec3 d3 = dx * dx + dy * dy; // d31, d32 and d33, squared
              // Sort out the two smallest distances (F1, F2)
              vec3 d1a = min(d1, d2);
              d2 = max(d1, d2); // Swap to keep candidates for F2
              d2 = min(d2, d3); // neither F1 nor F2 are now in d3
              d1 = min(d1a, d2); // F1 is now in d1
              d2 = max(d1a, d2); // Swap to keep candidates for F2
              d1.xy = (d1.x < d1.y) ? d1.xy : d1.yx; // Swap if smaller
              d1.xz = (d1.x < d1.z) ? d1.xz : d1.zx; // F1 is in d1.x
              d1.yz = min(d1.yz, d2.yz); // F2 is now not in d2.yz
              d1.y = min(d1.y, d1.z); // nor in  d1.z
              d1.y = min(d1.y, d2.x); // F2 is in d1.y, we're done.
              return sqrt(d1.xy);
          }

          vec4 process(vec2 uv)
          {
              float scale = 7.0;
              //vec2 cel = cellular(uv*vec2(scale));
              vec2 cel = cellular(uv*vec2(prop_scale));
              return vec4(vec3(cel.y - cel.x),1.0);
          }
        `;

		this.buildShader(source);
	}
}

// http://www.smart-page.net/smartnormal/js/SmartNormalMapFilter.js
export class TestNormalNode extends GpuDesignerNode {
	public init() {
		this.title = "Normal Map";
		//this.exportName = "result";
		this.addInput("height");

		this.addFloatProperty("strength", "Strength", 1, 0, 10, 0.01);

		const source = `
        vec4 process(vec2 uv)
        {
            //vec2 size = textureSize(height);
            vec2 size = vec2(1024,1024); // quick hack for now
            vec2 step = vec2(1.0,1.0)/size;

            // center point
            float d0 = abs(texture(height, uv + vec2(0.0, 0.0)).r);

            // sample horizontally
            float d1 = abs(texture(height, uv + vec2(step.x, 0.0)).r);
            float d2 = abs(texture(height, uv + vec2(-step.x, 0.0)).r);

            // sample vertically
            float d3 = abs(texture(height, uv + vec2(0.0, step.y)).r);
            float d4 = abs(texture(height, uv + vec2(0.0, -step.y)).r);

            // find diff horizontally and average
            float dx = ((d2 - d0) + (d0 - d1)) * 0.5;

            // find diff vertically and average
            float dy = ((d4 - d0) + (d0 - d3)) * 0.5;

            // calculate normal
            //float strength = 0.05;
            //float strength = (10.0-prop_strength) * 0.01;
            vec3 normal = normalize(vec3(dx * prop_strength, dy * prop_strength, 1.0));
            vec3 final = normal.xyz * 0.5 + 0.5; // bring to 0.0 - 1.0 range

            return vec4(final, 1.0);
        }
        `;

		this.buildShader(source);
	}
}

export class TestWarpNode extends GpuDesignerNode {
	public init() {
		this.title = "Warp";

		this.addInput("inputImage");
		this.addInput("height");

		this.addFloatProperty("strength", "Strength", 1, 0, 100, 0.01);

		// calculates normal, then warps uv by it
		const source = `
        vec4 process(vec2 uv)
        {
            //vec2 size = textureSize(height);
            vec2 size = vec2(1024,1024); // quick hack for now
            vec2 step = vec2(1.0,1.0)/size;

            // center point
            float d0 = abs(texture(height, uv + vec2(0.0, 0.0)).r);

            // sample horizontally
            float d1 = abs(texture(height, uv + vec2(step.x, 0.0)).r);
            float d2 = abs(texture(height, uv + vec2(-step.x, 0.0)).r);

            // sample vertically
            float d3 = abs(texture(height, uv + vec2(0.0, step.y)).r);
            float d4 = abs(texture(height, uv + vec2(0.0, -step.y)).r);

            // find diff horizontally and average
            float dx = ((d2 - d0) + (d0 - d1)) * 0.5;

            // find diff vertically and average
            float dy = ((d4 - d0) + (d0 - d3)) * 0.5;

            // calculate normal
            //float strength = 0.05;
            //float strength = (10.0-prop_strength) * 0.01;
            vec2 offset = vec2(dx, dy);
            
            vec4 color = texture(inputImage, uv + offset * prop_strength);

            return color;
        }
        `;

		this.buildShader(source);
	}
}

// https://thebookofshaders.com/11/
export class TestSimplexNode extends GpuDesignerNode {
	public init() {
		this.title = "Simplex Noise";

		this.addFloatProperty("scale", "Scale", 100, 1, 1000, 0.01);

		const source = `
        float random (in vec2 st) {
            return fract(sin(dot(st.xy,
                                 vec2(12.9898,78.233)))
                         * 43758.5453123);
        }

        float noise (in vec2 st) {
            vec2 i = floor(st);
            vec2 f = fract(st);
        
            // Four corners in 2D of a tile
            float a = random(i);
            float b = random(i + vec2(1.0, 0.0));
            float c = random(i + vec2(0.0, 1.0));
            float d = random(i + vec2(1.0, 1.0));
        
            // Smooth Interpolation
        
            // Cubic Hermine Curve.  Same as SmoothStep()
            vec2 u = f*f*(3.0-2.0*f);
            // u = smoothstep(0.,1.,f);
        
            // Mix 4 coorners porcentages
            return mix(a, b, u.x) +
                    (c - a)* u.y * (1.0 - u.x) +
                    (d - b) * u.x * u.y;
        }

        vec2 skew (vec2 st) {
            vec2 r = vec2(0.0);
            r.x = 1.1547*st.x;
            r.y = st.y+0.5*r.x;
            return r;
        }
        
        vec3 simplexGrid (vec2 st) {
            vec3 xyz = vec3(0.0);
        
            vec2 p = fract(skew(st));
            if (p.x > p.y) {
                xyz.xy = 1.0-vec2(p.x,p.y-p.x);
                xyz.z = p.y;
            } else {
                xyz.yz = 1.0-vec2(p.x-p.y,p.y);
                xyz.x = p.x;
            }
        
            return fract(xyz);
        }

        vec4 process(vec2 uv)
        {
            vec3 color = vec3(noise(uv * prop_scale));

            return vec4(color,1.0);
        }
        `;

		this.buildShader(source);
	}
}

export class TestBrickNode extends GpuDesignerNode {
	public init() {
		this.title = "Brick";

		this.addIntProperty("widthX", "Width X", 3, 0, 10, 1);
		this.addIntProperty("widthY", "Width Y", 6, 0, 10, 1);

		this.addFloatProperty("holeX", "Hole X", 2, 1, 10, 0.01);
		this.addFloatProperty("holeY", "Hole Y", 2, 1, 10, 0.01);

		const source = `
        vec4 process(vec2 uv)
        {
            float u = uv.x;
            float v = uv.y;

            float widthX = float(prop_widthX);
            float widthY = float(prop_widthY);
            float shiftX = 0.5;
            float shiftRandWeight = 0.0;
            //float holeX = 0.03;
            float holeX = prop_holeX / 100.0;
            //float holeY = 0.03;
            float holeY = prop_holeY / 100.0;
            //float smooth = 0.05;
            float smooth = 0.01;

            float normX = widthX;
            float normY = widthY;
            float uu = u * normX;
            float vv = v * normY;

            float y = vv - floor(vv);

            if (vv >= normY) vv -= normY;
            if (uu >= normX) uu -= normX;
            
            float by = floor(vv);

            if ((vv * 0.5 - floor(vv * 0.5)) >= 0.5)
                uu += shiftX;
            
            if (uu >= normX) uu -= normX;

            float bx = floor(uu);

            float x = uu - floor(uu);

            float val = 1.0;

            bool inside = true;
            if (holeX > 0.0) inside = inside && (x > (holeX * normX));
            if (holeY > 0.0) inside = inside && (y > (holeY * normY));

            if (inside) {
                float dist = min(min(x -holeX*normX, 1.0 - x)/normX, min(y - holeY*normY, 1.0 - y)/normY);
                dist *= min(normX, normY);
                
                if (dist < smooth) {
                    val = val * dist/smooth;
                }
            } else val = 0.0;

            vec4 color;
            color.r = val;
            color.g = val;
            color.b = val;
            color.a = 1.0;

            return color;
        }
        `;

		this.buildShader(source);
	}
}
