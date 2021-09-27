import {
	compileShader,
	buildShaderProgram,
	createTextureWithType,
	TextureDataType
} from "./gl";

// to be used mainly to extract pixels from textures
// to the needed format for export
export class TextureDataConverter {
	gl: WebGL2RenderingContext;
	readFbo: WebGLFramebuffer;
	shaderProgram: WebGLProgram;
	texture: WebGLTexture;

	posBuffer: WebGLBuffer;
	texCoordBuffer: WebGLBuffer;

	constructor(gl: WebGL2RenderingContext) {
		this.gl = gl;
		this.readFbo = this.gl.createFramebuffer();
		this.createVertexBuffers();
		this.buildShader();
	}

	getData(
		sourceTexture: WebGLTexture,
		width: number,
		height: number,
		dataType: TextureDataType,
		flipY: boolean = false
	): ArrayBuffer {
		const gl = this.gl;

		// create textures based on specified data type
		this.texture = createTextureWithType(
			this.gl,
			TextureDataType.Uint16,
			width,
			height
		);

		// set fbo and bind newly created texture
		gl.bindFramebuffer(gl.FRAMEBUFFER, this.readFbo);
		gl.activeTexture(gl.TEXTURE0);
		gl.framebufferTexture2D(
			gl.FRAMEBUFFER,
			gl.COLOR_ATTACHMENT0,
			gl.TEXTURE_2D,
			this.texture,
			0
		);

		gl.viewport(0, 0, width, height);
		gl.disable(gl.DEPTH_TEST);
		gl.clearColor(0, 0, 0, 1);
		gl.clear(gl.COLOR_BUFFER_BIT);

		// bind shader
		gl.useProgram(this.shaderProgram);

		// clear all inputs
		gl.activeTexture(gl.TEXTURE0);
		gl.bindTexture(gl.TEXTURE_2D, sourceTexture);

		gl.uniform1i(gl.getUniformLocation(this.shaderProgram, "tex"), 0);

		gl.uniform1i(
			gl.getUniformLocation(this.shaderProgram, "flipY"),
			flipY ? 1 : 0
		);

		// render texure to fbo
		const posLoc = gl.getAttribLocation(this.shaderProgram, "a_pos");
		const texCoordLoc = gl.getAttribLocation(this.shaderProgram, "a_texCoord");

		// provide texture coordinates for the rectangle.
		gl.bindBuffer(gl.ARRAY_BUFFER, this.posBuffer);
		gl.enableVertexAttribArray(posLoc);
		gl.vertexAttribPointer(posLoc, 3, gl.FLOAT, false, 0, 0);

		gl.bindBuffer(gl.ARRAY_BUFFER, this.texCoordBuffer);
		gl.enableVertexAttribArray(texCoordLoc);
		gl.vertexAttribPointer(texCoordLoc, 2, gl.FLOAT, false, 0, 0);

		gl.drawArrays(gl.TRIANGLES, 0, 6);

		gl.disableVertexAttribArray(posLoc);
		gl.disableVertexAttribArray(texCoordLoc);

		// read back pixels

		let readDataType = gl.UNSIGNED_BYTE;
		let arrayBufferView: ArrayBufferView = null;
		let arrayBuffer: ArrayBuffer = null;

		switch (dataType) {
			case TextureDataType.Uint8:
				readDataType = gl.UNSIGNED_BYTE;
				arrayBuffer = new ArrayBuffer(width * height * 4 * 1);
				arrayBufferView = new Uint8Array(arrayBuffer);
				break;
			case TextureDataType.Uint16:
				readDataType = gl.UNSIGNED_SHORT;
				arrayBuffer = new ArrayBuffer(width * height * 4 * 2);
				arrayBufferView = new Uint16Array(arrayBuffer);
				break;
			case TextureDataType.Float16:
				readDataType = gl.HALF_FLOAT;
				arrayBuffer = new ArrayBuffer(width * height * 4 * 2);
				arrayBufferView = new Uint16Array(arrayBuffer);
				break;
			case TextureDataType.Float32:
				readDataType = gl.FLOAT;
				arrayBuffer = new ArrayBuffer(width * height * 4 * 4);
				arrayBufferView = new Float32Array(arrayBuffer);
				break;
		}

		if (gl.checkFramebufferStatus(gl.FRAMEBUFFER) === gl.FRAMEBUFFER_COMPLETE) {
			gl.readPixels(
				0,
				0,
				width,
				height,
				gl.RGBA,
				readDataType,
				arrayBufferView
			);

			// todo: check for errors in this operation
			// gl.readPixels(
			// 	0,
			// 	0,
			// 	this.designer.width,
			// 	this.designer.height,
			// 	gl.RGBA,
			// 	gl.HALF_FLOAT,
			// 	data
			// );
		} else {
			alert("getPixelData: unable to read from FBO");
		}

		// cleanup
		gl.enable(gl.DEPTH_TEST);
		gl.bindFramebuffer(gl.FRAMEBUFFER, null);

		return arrayBuffer;
	}

	createVertexBuffers() {
		const gl = this.gl;
		//var texCoordLocation = gl.getAttribLocation(program, "a_texCoord");

		// provide texture coordinates for the rectangle.
		this.texCoordBuffer = gl.createBuffer();
		gl.bindBuffer(gl.ARRAY_BUFFER, this.texCoordBuffer);
		gl.bufferData(
			gl.ARRAY_BUFFER,
			new Float32Array([
				0.0,
				0.0,
				1.0,
				0.0,
				0.0,
				1.0,
				0.0,
				1.0,
				1.0,
				0.0,
				1.0,
				1.0
			]),
			gl.STATIC_DRAW
		);
		//gl.enableVertexAttribArray(texCoordLocation);
		//gl.vertexAttribPointer(texCoordLocation, 2, gl.FLOAT, false, 0, 0);

		this.posBuffer = gl.createBuffer();
		gl.bindBuffer(gl.ARRAY_BUFFER, this.posBuffer);
		gl.bufferData(
			gl.ARRAY_BUFFER,
			new Float32Array([
				-1.0,
				-1.0,
				0.0,
				1.0,
				-1.0,
				0.0,
				-1.0,
				1.0,
				0.0,
				-1.0,
				1.0,
				0.0,
				1.0,
				-1.0,
				0.0,
				1.0,
				1.0,
				0.0
			]),
			gl.STATIC_DRAW
		);

		gl.bindBuffer(gl.ARRAY_BUFFER, null);
	}

	buildShader() {
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

		// https://stackoverflow.com/questions/51101023/render-to-16bits-unsigned-integer-2d-texture-in-webgl2
		// https://stackoverflow.com/questions/27509285/how-to-render-to-a-unsigned-integer-format
		let fragSource = `#version 300 es
        precision highp float;
        in vec2 v_texCoord;

        uniform sampler2D tex;
        // uniform vec2 _textureSize;
        uniform bool flipY;

        // out uvec4 fragColor;
        out vec4 fragColor;
            
        void main() {
            vec2 texCoords = vec2(v_texCoord.x, flipY? 1.0 - v_texCoord.y:v_texCoord.y);
			vec4 result = texture(tex, texCoords);
			// fragColor = uvec4(65535.0, 65535.0, 0, 65535.0);
			fragColor = vec4(result);
			// fragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }

        `;

		this.shaderProgram = buildShaderProgram(this.gl, vertSource, fragSource);
	}
}
