export function createTexture(
	gl: WebGLRenderingContext,
	width: number,
	height: number
): WebGLTexture {
	const texture = gl.createTexture();
	gl.bindTexture(gl.TEXTURE_2D, texture);

	// Set the parameters so we can render any size image.
	//gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
	//gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

	// Upload the image into the texture.
	gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, null);

	return texture;
}

export function getShaderSource(id) {
	const shaderScript = document.getElementById(id);
	if (!shaderScript) {
		return null;
	}

	let str = "";
	let k = shaderScript.firstChild;
	while (k) {
		if (k.nodeType == 3) {
			str += k.textContent;
		}
		k = k.nextSibling;
	}

	return str;
}

export function compileShader(gl, source, shaderType) {
	const shader = gl.createShader(shaderType);

	gl.shaderSource(shader, source);
	gl.compileShader(shader);

	if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
		alert(gl.getShaderInfoLog(shader));
		return null;
	}

	return shader;
}

export function buildShaderProgram(gl, vertSource, fragSource) {
	const vertexShader = compileShader(gl, vertSource, gl.VERTEX_SHADER);
	const fragmentShader = compileShader(gl, fragSource, gl.FRAGMENT_SHADER);

	const shaderProgram = gl.createProgram();
	gl.attachShader(shaderProgram, vertexShader);
	gl.attachShader(shaderProgram, fragmentShader);
	gl.linkProgram(shaderProgram);

	if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
		alert("Could not initialise shaders");
	}

	gl.useProgram(shaderProgram);
	/*
    shaderProgram.samplerLoc = gl.getUniformLocation(shaderProgram, "tex");
 
    shaderProgram.vertexPositionAttribute = gl.getAttribLocation(shaderProgram, "aVertexPosition");
    gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);
 
    shaderProgram.vertexUVAttribute = gl.getAttribLocation(shaderProgram, "aVertexUV");
    gl.enableVertexAttribArray(shaderProgram.vertexUVAttribute);
    */

	return shaderProgram;
}
