/*
NOTES:
Unity's mettalic and gloss maps are in one texture.
*/
import JSZip from "jszip";

var TextureType = {
  Default: 0,
  NormalMap: 1
};

var exporter = {
  canvas: null,
  gl: null,
  posBuffer: null,
  texCoordBuffer: null,

  // shaders
  metalGlossProgram: null,
  normalProgram: null
};

export function unityZipExport(editor, materialName) {
  var zip = new JSZip();

  var matData = {
    name: materialName,
    keywords: "",
    albedo_tex: matNullTexTemp,
    metallicgloss_tex: matNullTexTemp,
    normal_tex: matNullTexTemp,
    height_tex: matNullTexTemp
  };

  // write albedo first
  if (editor.hasTextureChannel("albedo")) {
    var albedoGuid = newGuid();
    zip.file(
      "albedo.png",
      canvasToBase64(editor.getChannelCanvasImage("albedo").canvas),
      { base64: true }
    );
  }

  if (editor.hasTextureChannel("normal")) {
    var normalCanvas = editor.getChannelCanvasImage("albedo");
    exporter.canvas.width = normalCanvas.width();
    exporter.canvas.height = normalCanvas.height();

    fixNormalMap(exporter, normalCanvas.createTexture(exporter.gl));
    zip.file("normal.png", canvasToBase64(exporter.canvas), {
      base64: true
    });
  }

  if (
    editor.hasTextureChannel("metalness") ||
    editor.hasTextureChannel("roughness")
  ) {
    exporter.canvas.width = editor.getImageWidth();
    exporter.canvas.height = editor.getImageHeight();

    var mTex = null;
    if (editor.hasTextureChannel("metalness"))
      mTex = editor
        .getChannelCanvasImage("metalness")
        .createTexture(exporter.gl);

    var rTex = null;
    if (editor.hasTextureChannel("roughness"))
      rTex = editor
        .getChannelCanvasImage("roughness")
        .createTexture(exporter.gl);

    generateMetallicGloss(exporter, mTex, rTex);
    zip.file("metallic_gloss.png", canvasToBase64(exporter.canvas), {
      base64: true
    });
  }

  return zip.generateAsync({ type: "nodebuffer" });
}

// creates canvas and context
// creates shaders for converting the the textures
function initGLAndResources(exporter) {
  var canvas = document.createElement("canvas");
  var gl = canvas.getContext("webgl");

  exporter.canvas = canvas;
  exporter.gl = gl;

  exporter.metalGlossProgram = buildShaderProgram(
    gl,
    DEFAULT_VERT,
    METALLICGLOSS_FRAG
  );
  exporter.normalProgram = buildShaderProgram(gl, DEFAULT_VERT, NORMAl_FRAG);

  createVertexBuffers(exporter);
}

function getShaderSource(id) {
  var shaderScript = document.getElementById(id);
  if (!shaderScript) {
    return null;
  }

  var str = "";
  var k = shaderScript.firstChild;
  while (k) {
    if (k.nodeType == 3) {
      str += k.textContent;
    }
    k = k.nextSibling;
  }

  return str;
}

function compileShader(gl, source, shaderType) {
  var shader = gl.createShader(shaderType);

  gl.shaderSource(shader, source);
  gl.compileShader(shader);

  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    alert(gl.getShaderInfoLog(shader));
    return null;
  }

  return shader;
}

function buildShaderProgram(gl, vertSource, fragSource) {
  var vertexShader = compileShader(gl, vertSource, gl.VERTEX_SHADER);
  var fragmentShader = compileShader(gl, fragSource, gl.FRAGMENT_SHADER);

  var shaderProgram = gl.createProgram();
  gl.attachShader(shaderProgram, vertexShader);
  gl.attachShader(shaderProgram, fragmentShader);
  gl.linkProgram(shaderProgram);

  if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
    alert("Could not initialise shaders");
  }

  gl.useProgram(shaderProgram);

  return shaderProgram;
}

//var texCoordBuffer = new WebGLBuffer;
//var posBuffer = new WebGLBuffer;

// render quad using shader and texture inputs
// returns HtmlImageElement
function renderToImage(exporter, program, inputs) {
  var gl = exporter.gl;

  gl.viewport(0, 0, exporter.canvas.width, exporter.canvas.height);

  // bind texture to fbo
  //gl.clearColor(0,0,1,1);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

  // bind shader
  gl.useProgram(program);

  // pass textures
  var texIndex = 0;
  for (var i in inputs) {
    var input = inputs[i];
    if (input.tex) {
      gl.activeTexture(gl.TEXTURE0 + texIndex);
      gl.bindTexture(gl.TEXTURE_2D, input.tex);
      gl.uniform1i(gl.getUniformLocation(program, input.name), texIndex);
      gl.uniform1i(gl.getUniformLocation(program, input.name + "Enabled"), 1);
    } else {
      gl.uniform1i(gl.getUniformLocation(program, input.name + "Enabled"), 0);
    }
    texIndex++;
  }

  // bind mesh
  var posLoc = gl.getAttribLocation(program, "a_pos");
  var texCoordLoc = gl.getAttribLocation(program, "a_texCoord");

  // provide texture coordinates for the rectangle.
  gl.bindBuffer(gl.ARRAY_BUFFER, exporter.posBuffer);
  gl.enableVertexAttribArray(posLoc);
  gl.vertexAttribPointer(posLoc, 3, gl.FLOAT, false, 0, 0);

  gl.bindBuffer(gl.ARRAY_BUFFER, exporter.texCoordBuffer);
  gl.enableVertexAttribArray(texCoordLoc);
  gl.vertexAttribPointer(texCoordLoc, 2, gl.FLOAT, false, 0, 0);

  gl.drawArrays(gl.TRIANGLES, 0, 6);

  gl.disableVertexAttribArray(posLoc);
  gl.disableVertexAttribArray(texCoordLoc);
}

function createVertexBuffers(exporter) {
  var gl = exporter.gl;

  // provide texture coordinates for the rectangle.
  var texCoordBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, texCoordBuffer);
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

  var posBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, posBuffer);
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

  exporter.texCoordBuffer = texCoordBuffer;
  exporter.posBuffer = posBuffer;
}

function canvasToBase64(canvas) {
  var data = canvas.toDataURL();
  // todo: maybe script header?
  // https://code-examples.net/en/q/6f412f
  data = data.replace(/^data:image\/(png|jpg);base64,/, "");
  return data;
}

// todo: cleanup textures!!

// initialize
initGLAndResources(exporter);
