import {
	compileShader,
	buildShaderProgram,
	createTextureWithType,
	TextureDataType
} from "./gl";

export enum TextureComponents {
	RGBA = "RGBA",
	RGB = "RGB",
	R = "R",
	G = "G",
	B = "B",
	A = "A"
}

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
		components: TextureComponents = TextureComponents.RGBA,
		flipY: boolean = false
	): ArrayBuffer {
		const gl = this.gl;

		// create textures based on specified data type
		this.texture = createTextureWithType(this.gl, dataType, width, height);

		// set fbo and bind newly created texture
		gl.bindFramebuffer(gl.FRAMEBUFFER, this.readFbo);
		gl.activeTexture(gl.TEXTURE0);

		// NOTE: texture must be in the RGBA format
		// spec doesn't mandate any other format being supported
		// https://www.html5gamedevs.com/topic/38191-how-to-use-webgl2-rgb-texture-format/
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

		// determine which channels the source comes from
		// 0 - all
		// 1 - red
		// 2 - green
		// 3 - blue
		// 4 - alpha
		let outputChannel: number = 0;

		switch (components) {
			case TextureComponents.R:
				outputChannel = 1;
				break;
			case TextureComponents.G:
				outputChannel = 2;
				break;
			case TextureComponents.B:
				outputChannel = 3;
				break;
			case TextureComponents.A:
				outputChannel = 4;
				break;
		}

		gl.uniform1i(
			gl.getUniformLocation(this.shaderProgram, "channel"),
			outputChannel
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

		let format: number = gl.RGBA;
		let numComponents: number = 4;

		// commented this out since the format has to be
		// RGBA. Any other format and WebGL throws an error
		// switch (components) {
		// 	case TextureComponents.RGBA:
		// 		numComponents = 4;
		// 		format = gl.RGBA;
		// 		break;
		// 	case TextureComponents.RGB:
		// 		numComponents = 3;
		// 		format = gl.RGB;
		// 		break;
		// 	case TextureComponents.R:
		// 	case TextureComponents.G:
		// 	case TextureComponents.B:
		// 	case TextureComponents.A:
		// 		numComponents = 1;
		// 		format = gl.RED;
		// 		break;
		// }

		switch (dataType) {
			case TextureDataType.Uint8:
				readDataType = gl.UNSIGNED_BYTE;
				arrayBuffer = new ArrayBuffer(width * height * numComponents * 1);
				arrayBufferView = new Uint8Array(arrayBuffer);
				break;
			case TextureDataType.Uint16:
				readDataType = gl.UNSIGNED_SHORT;
				arrayBuffer = new ArrayBuffer(width * height * numComponents * 2);
				arrayBufferView = new Uint16Array(arrayBuffer);
				break;
			case TextureDataType.Float16:
				readDataType = gl.HALF_FLOAT;
				arrayBuffer = new ArrayBuffer(width * height * numComponents * 2);
				arrayBufferView = new Uint16Array(arrayBuffer);
				break;
			case TextureDataType.Float32:
				readDataType = gl.FLOAT;
				arrayBuffer = new ArrayBuffer(width * height * numComponents * 4);
				arrayBufferView = new Float32Array(arrayBuffer);
				break;
		}

		if (gl.checkFramebufferStatus(gl.FRAMEBUFFER) === gl.FRAMEBUFFER_COMPLETE) {
			gl.readPixels(0, 0, width, height, format, readDataType, arrayBufferView);

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

		// convert buffer to required format
		// currently only supports uint8 and uint16

		if (dataType === TextureDataType.Uint8) {
			if (components === TextureComponents.RGB) {
				arrayBuffer = rgbaToRgbUint8(arrayBuffer, width, height);
			} else if (
				components === TextureComponents.R ||
				components === TextureComponents.G ||
				components === TextureComponents.B ||
				components === TextureComponents.A
			) {
				arrayBuffer = rgbaToRUint8(arrayBuffer, width, height);
			}
		} else if (dataType === TextureDataType.Uint16) {
			if (components === TextureComponents.RGB) {
				arrayBuffer = rgbaToRgbUint16(arrayBuffer, width, height);
			}
			if (
				components === TextureComponents.R ||
				components === TextureComponents.G ||
				components === TextureComponents.B ||
				components === TextureComponents.A
			) {
				arrayBuffer = rgbaToRUint16(arrayBuffer, width, height);
			}
		}

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

		uniform int channel;

        // out uvec4 fragColor;
        out vec4 fragColor;
            
        void main() {
            vec2 texCoords = vec2(v_texCoord.x, flipY? 1.0 - v_texCoord.y:v_texCoord.y);
			vec4 result = texture(tex, texCoords);

			// determine which channels to extract if any
			switch(channel){
				// 0 - do nothing

				// red
				case 1:
					result = vec4(result.r);
					break;
				// green
				case 2:
					result = vec4(result.g);
					break;
				// blue
				case 3:
					result = vec4(result.b);
					break;
				// alpha
				case 4:
					result = vec4(result.a);
					break;
			}

			fragColor = result;
			// fragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }

        `;

		this.shaderProgram = buildShaderProgram(this.gl, vertSource, fragSource);
	}
}

function rgbaToRgbUint8(
	buffer: ArrayBuffer,
	width: number,
	height: number
): ArrayBuffer {
	const totalPixels = width * height;
	const numComponents = 3;
	const inputArray: Uint8Array = new Uint8Array(buffer);
	const outputArray: Uint8Array = new Uint8Array(totalPixels * numComponents);

	for (let i = 0; i < totalPixels; i++) {
		outputArray[i * numComponents + 0] = inputArray[i * 4 + 0];
		outputArray[i * numComponents + 1] = inputArray[i * 4 + 1];
		outputArray[i * numComponents + 2] = inputArray[i * 4 + 2];
	}

	return outputArray.buffer;
}

function rgbaToRUint8(
	buffer: ArrayBuffer,
	width: number,
	height: number
): ArrayBuffer {
	const totalPixels = width * height;
	const numComponents = 1;
	const inputArray: Uint8Array = new Uint8Array(buffer);
	const outputArray: Uint8Array = new Uint8Array(totalPixels * numComponents);

	for (let i = 0; i < totalPixels; i++) {
		outputArray[i * numComponents] = inputArray[i * 4];
	}

	return outputArray.buffer;
}

function rgbaToRgbUint16(
	buffer: ArrayBuffer,
	width: number,
	height: number
): ArrayBuffer {
	const totalPixels = width * height;
	const numComponents = 3;
	const inputArray: Uint16Array = new Uint16Array(buffer);
	const outputArray: Uint16Array = new Uint16Array(totalPixels * numComponents);

	for (let i = 0; i < totalPixels; i++) {
		outputArray[i * numComponents + 0] = inputArray[i * 4 + 0];
		outputArray[i * numComponents + 1] = inputArray[i * 4 + 1];
		outputArray[i * numComponents + 2] = inputArray[i * 4 + 2];
	}

	return outputArray.buffer;
}

function rgbaToRUint16(
	buffer: ArrayBuffer,
	width: number,
	height: number
): ArrayBuffer {
	const totalPixels = width * height;
	const numComponents = 1;
	const inputArray: Uint16Array = new Uint16Array(buffer);
	const outputArray: Uint16Array = new Uint16Array(totalPixels * numComponents);

	for (let i = 0; i < totalPixels; i++) {
		outputArray[i * numComponents] = inputArray[i * 4];
	}

	return outputArray.buffer;
}
