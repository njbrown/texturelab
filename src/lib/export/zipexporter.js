import AdmZip from "adm-zip";
import electron from "electron";

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

export async function zipExport(editor, materialName) {
  let zip = new AdmZip();

  // write albedo first
  // if (editor.hasTextureChannel("albedo")) {
  //   zip.addFile(
  //     "albedo.png",
  //     canvasToBuffer(editor.getChannelCanvasImage("albedo").canvas)
  //   );
  // }

  for (let channel in editor.textureChannels) {
    if (editor.textureChannels.hasOwnProperty(channel)) {
      zip.addFile(
        channel + ".png",
        canvasToBuffer(editor.getChannelCanvasImage(channel).canvas)
      );
    }
  }

  return zip;
}

// merges metallic and inverted roughness map
// uses a metallic of 0 if no metallic map is provided
// sets glosiness to 0 by default
function generateMetallicGloss(exporter, mTex, rTex) {
  renderToImage(exporter, exporter.metalGlossProgram, [
    { name: "u_metallicMap", tex: mTex },
    { name: "u_roughnessMap", tex: rTex }
  ]);
}

// inverts normal map
function fixNormalMap(exporter, tex) {
  renderToImage(exporter, exporter.normalProgram, [
    { name: "u_normalMap", tex: tex }
  ]);
}

const DEFAULT_VERT = `precision mediump float;
    
    attribute vec3 a_pos;
    attribute vec2 a_texCoord;
        
    // the texCoords passed in from the vertex shader.
    varying vec2 v_texCoord;
        
    void main() {
        gl_Position = vec4(a_pos,1.0);
        v_texCoord = a_texCoord;
    }`;

const NORMAl_FRAG = `precision mediump float;
    varying vec2 v_texCoord;
    uniform sampler2D u_normalMap;
        
    void main() {
        vec4 norm = texture2D(u_normalMap,vec2(v_texCoord.x, 1.0 - v_texCoord.y));
        norm.z = 1.0 - norm.z;
        gl_FragColor = norm;
    }`;

const METALLICGLOSS_FRAG = `precision mediump float;
    varying vec2 v_texCoord;
    uniform sampler2D u_metallicMap;
    uniform bool u_metallicMapEnabled;
    uniform sampler2D u_roughnessMap;
    uniform bool u_roughnessMapEnabled;
        
    void main() {
        vec2 texCoord = vec2(v_texCoord.x, 1.0 - v_texCoord.y);
        float metal = 0.0;
        if (u_metallicMapEnabled)
             metal = texture2D(u_metallicMap,texCoord).r;
    
        
        float gloss = 0.0;
        if (u_roughnessMapEnabled)
            gloss = 1.0 - texture2D(u_roughnessMap,texCoord).r;
    
        gl_FragColor = vec4(vec3(metal), gloss);
    }`;

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

function canvasToBuffer(canvas) {
  // https://github.com/mattdesl/electron-canvas-to-buffer/blob/master/index.js
  const url = canvas.toDataURL("image/png", 1);
  const nativeImage = electron.nativeImage.createFromDataURL(url);
  const buffer = nativeImage.toPNG();

  return buffer;
}

// todo: cleanup textures!!

// initialize
initGLAndResources(exporter);
