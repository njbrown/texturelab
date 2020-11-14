import { Guid } from "../utils";
import { Designer, NodeRenderContext } from "../designer";
import {
	Property,
	FloatProperty,
	IntProperty,
	BoolProperty,
	EnumProperty,
	ColorProperty,
	StringProperty,
	GradientProperty,
	IPropertyHolder
} from "./properties";
import { buildShaderProgram } from "./gl";
import { Color } from "./color";
import { Gradient } from "./gradient";
import { DesignerNode } from "./designernode";

export class GpuDesignerNode extends DesignerNode {
	tex: WebGLTexture;
	//program:WebGLShader;
	source: string; // shader code
	shaderProgram: WebGLProgram;

	public isCpu() {
		return false;
	}

	public render(context: NodeRenderContext) {
		const inputs = context.inputs;
		const gl = this.gl;

		gl.bindFramebuffer(gl.FRAMEBUFFER, context.fbo);
		gl.activeTexture(gl.TEXTURE0);
		gl.framebufferTexture2D(
			gl.FRAMEBUFFER,
			gl.COLOR_ATTACHMENT0,
			gl.TEXTURE_2D,
			this.tex,
			0
		);

		gl.viewport(0, 0, context.textureWidth, context.textureHeight);

		// bind texture to fbo
		//gl.clearColor(0,0,1,1);
		gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

		// bind shader
		gl.useProgram(this.shaderProgram);

		// clear all inputs
		for (const input of this.inputs) {
			gl.activeTexture(gl.TEXTURE0 + texIndex);
			gl.bindTexture(gl.TEXTURE_2D, null);

			gl.uniform1i(gl.getUniformLocation(this.shaderProgram, input), 0);

			gl.uniform1i(
				gl.getUniformLocation(this.shaderProgram, input + "_connected"),
				0
			);
		}

		// pass inputs for rendering
		var texIndex = 0;
		for (const input of inputs) {
			gl.activeTexture(gl.TEXTURE0 + texIndex);
			gl.bindTexture(gl.TEXTURE_2D, input.node.tex);
			gl.uniform1i(
				gl.getUniformLocation(this.shaderProgram, input.name),
				texIndex
			);
			gl.uniform1i(
				gl.getUniformLocation(this.shaderProgram, input.name + "_connected"),
				1
			);
			//console.log("bound texture " + texIndex);
			texIndex++;
		}

		// pass seed
		gl.uniform1f(
			gl.getUniformLocation(this.shaderProgram, "_seed"),
			this.designer.getRandomSeed()
		);

		// texture size
		gl.uniform2f(
			gl.getUniformLocation(this.shaderProgram, "_textureSize"),
			this.designer.width,
			this.designer.height
		);

		// pass properties
		for (const prop of this.properties) {
			if (prop instanceof FloatProperty) {
				gl.uniform1f(
					gl.getUniformLocation(this.shaderProgram, "prop_" + prop.name),
					(prop as FloatProperty).value
				);
			}
			if (prop instanceof IntProperty) {
				gl.uniform1i(
					gl.getUniformLocation(this.shaderProgram, "prop_" + prop.name),
					(prop as IntProperty).value
				);
			}
			if (prop instanceof BoolProperty) {
				gl.uniform1i(
					gl.getUniformLocation(this.shaderProgram, "prop_" + prop.name),
					(prop as BoolProperty).value == false ? 0 : 1
				);
			}
			if (prop instanceof EnumProperty) {
				gl.uniform1i(
					gl.getUniformLocation(this.shaderProgram, "prop_" + prop.name),
					(prop as EnumProperty).index
				);
			}
			if (prop instanceof ColorProperty) {
				const col = (prop as ColorProperty).value;
				//console.log("color: ", col);
				gl.uniform4f(
					gl.getUniformLocation(this.shaderProgram, "prop_" + prop.name),
					col.r,
					col.g,
					col.b,
					col.a
				);
			}
			if (prop instanceof GradientProperty) {
				const gradient = (prop as GradientProperty).value;

				gl.uniform1i(
					gl.getUniformLocation(
						this.shaderProgram,
						"prop_" + prop.name + ".numPoints"
					),
					gradient.points.length
				);

				for (let i = 0; i < gradient.points.length; i++) {
					const point = gradient.points[i];
					const col = point.color;

					gl.uniform3f(
						gl.getUniformLocation(
							this.shaderProgram,
							"prop_" + prop.name + ".colors[" + i + "]"
						),
						col.r,
						col.g,
						col.b
					);

					gl.uniform1f(
						gl.getUniformLocation(
							this.shaderProgram,
							"prop_" + prop.name + ".positions[" + i + "]"
						),
						point.t
					);
				}
			}
		}

		// bind mesh
		const posLoc = gl.getAttribLocation(this.shaderProgram, "a_pos");
		const texCoordLoc = gl.getAttribLocation(this.shaderProgram, "a_texCoord");

		// provide texture coordinates for the rectangle.
		gl.bindBuffer(gl.ARRAY_BUFFER, this.designer.posBuffer);
		gl.enableVertexAttribArray(posLoc);
		gl.vertexAttribPointer(posLoc, 3, gl.FLOAT, false, 0, 0);

		gl.bindBuffer(gl.ARRAY_BUFFER, this.designer.texCoordBuffer);
		gl.enableVertexAttribArray(texCoordLoc);
		gl.vertexAttribPointer(texCoordLoc, 2, gl.FLOAT, false, 0, 0);

		gl.drawArrays(gl.TRIANGLES, 0, 6);

		gl.disableVertexAttribArray(posLoc);
		gl.disableVertexAttribArray(texCoordLoc);

		gl.bindFramebuffer(gl.FRAMEBUFFER, null);
	}

	public _init() {
		//this.inputs = new Array();
		//this.properties = new Array();
		this.createTexture();

		this.init();
	}

	// #source gets appended to fragment shader
	buildShader(source: string) {
		const vertSource = `#version 300 es
        precision highp float;

        in vec3 a_pos;
        in vec2 a_texCoord;
            
        // the texCoords passed in from the vertex shader.
        out vec2 v_texCoord;
            
        void main() {
            gl_Position = vec4(a_pos,1.0);
            v_texCoord = a_texCoord;
        }`;

		let fragSource = `#version 300 es
        precision highp float;
        in vec2 v_texCoord;

        #define GRADIENT_MAX_POINTS 32

        vec4 process(vec2 uv);
        void initRandom();

        uniform vec2 _textureSize;

        out vec4 fragColor;
            
        void main() {
            initRandom();
			vec4 result = process(v_texCoord);
			fragColor = clamp(result, 0.0, 1.0);
        }

        `;

		fragSource =
			fragSource +
			this.createRandomLib() +
			this.createGradientLib() +
			this.createCodeForInputs() +
			this.createCodeForProps() +
			"#line 0\n" +
			source;

		this.shaderProgram = buildShaderProgram(this.gl, vertSource, fragSource);
	}

	createRandomLibOld(): string {
		// float _seed = `+this.designer.getRandomSeed().toFixed(1)+`;
		const code = `
        // this offsets the random start (should be a uniform)
        uniform float _seed;
        // this is the starting number for the rng
        // (should be set from the uv coordinates so it's unique per pixel)
        vec2 _randomStart;

        float _rand(vec2 co){
            return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
        }

        //todo: test variance!
        vec2 _rand2(vec2 co){
            return vec2(_rand(co), _rand(co + vec2(0.0001, 0.0001)));
        }

        float randomFloat(int index) 
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomVec2(int index) 
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomFloat(int index, float start, float end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + r*(end-start);
        }

        int randomInt(int index, int start, int end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + int(r*float(end-start));
        }

        bool randomBool(int index)
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index)) > 0.5;
        }

        void initRandom()
        {
            _randomStart = v_texCoord;
        }
        `;

		return code;
	}

	createRandomLib(): string {
		// float _seed = `+this.designer.getRandomSeed().toFixed(1)+`;
		const code = `
        // this offsets the random start (should be a uniform)
        uniform float _seed;
        // this is the starting number for the rng
        // (should be set from the uv coordinates so it's unique per pixel)
        vec2 _randomStart;

        // gives a much better distribution at 1
        #define RANDOM_ITERATIONS 1

        #define HASHSCALE1 443.8975
        #define HASHSCALE3 vec3(443.897, 441.423, 437.195)
        #define HASHSCALE4 vec4(443.897, 441.423, 437.195, 444.129)

        //  1 out, 2 in...
        float hash12(vec2 p)
        {
            vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
            p3 += dot(p3, p3.yzx + 19.19);
            return fract((p3.x + p3.y) * p3.z);
        }

        ///  2 out, 2 in...
        vec2 hash22(vec2 p)
        {
            vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
            p3 += dot(p3, p3.yzx+19.19);
            return fract((p3.xx+p3.yz)*p3.zy);

        }


        float _rand(vec2 uv)
        {
            float a = 0.0;
            for (int t = 0; t < RANDOM_ITERATIONS; t++)
            {
                float v = float(t+1)*.152;
                // 0.005 is a good value
                vec2 pos = (uv * v);
                a += hash12(pos);
            }

            return a/float(RANDOM_ITERATIONS);
        }

        vec2 _rand2(vec2 uv)
        {
            vec2 a = vec2(0.0);
            for (int t = 0; t < RANDOM_ITERATIONS; t++)
            {
                float v = float(t+1)*.152;
                // 0.005 is a good value
                vec2 pos = (uv * v);
                a += hash22(pos);
            }

            return a/float(RANDOM_ITERATIONS);
        }

        float randomFloat(int index) 
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomVec2(int index) 
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomFloat(int index, float start, float end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + r*(end-start);
        }

        int randomInt(int index, int start, int end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + int(r*float(end-start));
        }

        bool randomBool(int index)
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index)) > 0.5;
        }

        void initRandom()
        {
            _randomStart = v_texCoord;
        }
        `;

		return code;
	}

	createGradientLib(): string {
		// float _seed = `+this.designer.getRandomSeed().toFixed(1)+`;
		const code = `
    struct Gradient {
				vec3 colors[GRADIENT_MAX_POINTS];
				float positions[GRADIENT_MAX_POINTS];
				int numPoints;
    };
        
    // assumes points are sorted
    vec3 sampleGradient(vec3 colors[GRADIENT_MAX_POINTS], float positions[GRADIENT_MAX_POINTS], int numPoints, float t)
    {
        if (numPoints == 0)
            return vec3(1,0,0);
        
        if (numPoints == 1)
            return colors[0];
        
        // here at least two points are available
        if (t <= positions[0])
            return colors[0];
        
        int last = numPoints - 1;
        if (t >= positions[last])
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

    vec3 sampleGradient(Gradient gradient, float t)
    {
      return sampleGradient(gradient.colors, gradient.positions, gradient.numPoints, t);
    }
    `;

		return code;
	}

	createCodeForInputs() {
		let code = "";

		for (const input of this.inputs) {
			code += "uniform sampler2D " + input + ";\n";
			code += "uniform bool " + input + "_connected;\n";
		}

		return code;
	}

	createCodeForProps() {
		let code = "";

		//console.log(this.properties);
		//console.log(typeof FloatProperty);

		for (const prop of this.properties) {
			//code += "uniform sampler2D " + input + ";\n";
			if (prop instanceof FloatProperty) {
				code += "uniform float prop_" + prop.name + ";\n";
			}
			if (prop instanceof IntProperty) {
				code += "uniform int prop_" + prop.name + ";\n";
			}
			if (prop instanceof BoolProperty) {
				code += "uniform bool prop_" + prop.name + ";\n";
			}
			if (prop instanceof EnumProperty) {
				code += "uniform int prop_" + prop.name + ";\n";
			}
			if (prop instanceof ColorProperty) {
				code += "uniform vec4 prop_" + prop.name + ";\n";
			}
			if (prop instanceof GradientProperty) {
				// code += "uniform struct prop_" + prop.name + " {\n";
				// code += "vec3 colors[GRADIENT_MAX_POINTS];\n";
				// code += "vec3 positions[GRADIENT_MAX_POINTS];\n";
				// code += "int numPoints;\n";
				// code += "};";

				code += "uniform Gradient prop_" + prop.name + ";\n";
			}
		}

		code += "\n";

		return code;
	}
}
