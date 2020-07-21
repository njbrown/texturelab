/*
NOTES:
- _BumpMap is the normal map if then _NORMALMAP keyword is uses
    the normal map needs to be inverted to work in unity
- _METALLICGLOSSMAP keyword means the gloss map is in the alpha channel of the metallic map
    the gloss map is the inverse of the roughness map
    both metallic and gloss maps have to be combined into a single texture
    generate metallic of 0 if no metallic map is specified but a glossmap is
- _ParallaxMap is the height map and keyword _PARALLAXMAP is needed to enable it
*/
import JSZip from "jszip";

const keyWords = "_METALLICGLOSSMAP _NORMALMAP";

const materialTemplate = `%YAML 1.1
%TAG !u! tag:unity3d.com,2011:
--- !u!21 &2100000
Material:
    serializedVersion: 6
    m_ObjectHideFlags: 0
    m_PrefabParentObject: {fileID: 0}
    m_PrefabInternal: {fileID: 0}
    m_Name: {{name}}
    m_Shader: {fileID: 46, guid: 0000000000000000f000000000000000, type: 0}
    m_ShaderKeywords: {{keywords}}
    m_LightmapFlags: 4
    m_EnableInstancingVariants: 0
    m_DoubleSidedGI: 0
    m_CustomRenderQueue: -1
    stringTagMap: {}
    disabledShaderPasses: []
    m_SavedProperties:
      serializedVersion: 3
      m_TexEnvs:
      - _BumpMap:
          {{normal_tex}}
      - _DetailAlbedoMap:
          m_Texture: {fileID: 0}
          m_Scale: {x: 1, y: 1}
          m_Offset: {x: 0, y: 0}
      - _DetailMask:
          m_Texture: {fileID: 0}
          m_Scale: {x: 1, y: 1}
          m_Offset: {x: 0, y: 0}
      - _DetailNormalMap:
          m_Texture: {fileID: 0}
          m_Scale: {x: 1, y: 1}
          m_Offset: {x: 0, y: 0}
      - _EmissionMap:
          m_Texture: {fileID: 0}
          m_Scale: {x: 4, y: 4}
          m_Offset: {x: 0, y: 0}
      - _MainTex:
          {{albedo_tex}}
      - _MetallicGlossMap:
          {{metallicgloss_tex}}
      - _OcclusionMap:
          m_Texture: {fileID: 0}
          m_Scale: {x: 1, y: 1}
          m_Offset: {x: 0, y: 0}
      - _ParallaxMap:
          m_Texture: {fileID: 0}
          m_Scale: {x: 1, y: 1}
          m_Offset: {x: 0, y: 0}
      m_Floats:
      - _BumpScale: 1
      - _Cutoff: 0.5
      - _DetailNormalMapScale: 1
      - _DstBlend: 0
      - _GlossMapScale: 1
      - _Glossiness: 0.5
      - _GlossyReflections: 1
      - _Metallic: 0
      - _Mode: 0
      - _OcclusionStrength: 1
      - _Parallax: 0.02
      - _SmoothnessTextureChannel: 0
      - _SpecularHighlights: 1
      - _SrcBlend: 1
      - _UVSec: 0
      - _ZWrite: 1
      m_Colors:
      - _Color: {r: 1, g: 1, b: 1, a: 1}
      - _EmissionColor: {r: 0, g: 0, b: 0, a: 1}
`;

const materialMetaTemplate = `fileFormatVersion: 2
guid: {{material_guid}}
NativeFormatImporter:
    externalObjects: {}
    mainObjectFileID: 2100000
    userData: 
    assetBundleName: 
    assetBundleVariant: 
`;

const textureMetaTemplate = `fileFormatVersion: 2
guid: {{texture_guid}}
TextureImporter:
  fileIDToRecycleName: {}
  externalObjects: {}
  serializedVersion: 5
  mipmaps:
    mipMapMode: 0
    enableMipMap: 1
    sRGBTexture: 1
    linearTexture: 0
    fadeOut: 0
    borderMipMap: 0
    mipMapsPreserveCoverage: 0
    alphaTestReferenceValue: 0.5
    mipMapFadeDistanceStart: 1
    mipMapFadeDistanceEnd: 3
  bumpmap:
    convertToNormalMap: 0
    externalNormalMap: 0
    heightScale: 0.25
    normalMapFilter: 0
  isReadable: 0
  grayScaleToAlpha: 0
  generateCubemap: 6
  cubemapConvolution: 0
  seamlessCubemap: 0
  textureFormat: 1
  maxTextureSize: 2048
  textureSettings:
    serializedVersion: 2
    filterMode: -1
    aniso: 16
    mipBias: -1
    wrapU: -1
    wrapV: -1
    wrapW: -1
  nPOTScale: 1
  lightmap: 0
  compressionQuality: 50
  spriteMode: 0
  spriteExtrude: 1
  spriteMeshType: 1
  alignment: 0
  spritePivot: {x: 0.5, y: 0.5}
  spritePixelsToUnits: 100
  spriteBorder: {x: 0, y: 0, z: 0, w: 0}
  spriteGenerateFallbackPhysicsShape: 1
  alphaUsage: 1
  alphaIsTransparency: 0
  spriteTessellationDetail: -1
  textureType: {{type}}
  textureShape: 1
  singleChannelComponent: 0
  maxTextureSizeSet: 0
  compressionQualitySet: 0
  textureFormatSet: 0
  platformSettings:
  - serializedVersion: 2
    buildTarget: DefaultTexturePlatform
    maxTextureSize: 2048
    resizeAlgorithm: 0
    textureFormat: -1
    textureCompression: 1
    compressionQuality: 50
    crunchedCompression: 0
    allowsAlphaSplitting: 0
    overridden: 0
    androidETC2FallbackOverride: 0
  - serializedVersion: 2
    buildTarget: Standalone
    maxTextureSize: 2048
    resizeAlgorithm: 0
    textureFormat: -1
    textureCompression: 1
    compressionQuality: 50
    crunchedCompression: 0
    allowsAlphaSplitting: 0
    overridden: 0
    androidETC2FallbackOverride: 0
  - serializedVersion: 2
    buildTarget: Android
    maxTextureSize: 2048
    resizeAlgorithm: 0
    textureFormat: -1
    textureCompression: 1
    compressionQuality: 50
    crunchedCompression: 0
    allowsAlphaSplitting: 0
    overridden: 0
    androidETC2FallbackOverride: 0
  - serializedVersion: 2
    buildTarget: WebGL
    maxTextureSize: 2048
    resizeAlgorithm: 0
    textureFormat: -1
    textureCompression: 1
    compressionQuality: 50
    crunchedCompression: 0
    allowsAlphaSplitting: 0
    overridden: 0
    androidETC2FallbackOverride: 0
  spriteSheet:
    serializedVersion: 2
    sprites: []
    outline: []
    physicsShape: []
    bones: []
    spriteID: 
    vertices: []
    indices: 
    edges: []
    weights: []
  spritePackingTag: 
  userData: 
  assetBundleName: 
  assetBundleVariant: 
`;

const TextureType = {
	Default: 0,
	NormalMap: 1
};

const matNullTexTemp = `m_Texture: {fileID: 0}
        m_Scale: {x: 1, y: 1}
        m_Offset: {x: 0, y: 0}`;

const matTexTemp = `m_Texture: {fileID: 2800000, guid: {{guid}}, type: 3}
        m_Scale: {x: {{repeatx}}, y: {{repeaty}}}
        m_Offset: {x: 0, y: 0}`;

const exporter = {
	canvas: null,
	gl: null,
	posBuffer: null,
	texCoordBuffer: null,

	// shaders
	metalGlossProgram: null,
	normalProgram: null
};

export function unityExport(editor, materialName) {
	const zip = new JSZip();

	// todo : filter name, might have illegal characters
	//var materialName = saveData.name;

	const matData = {
		name: materialName,
		keywords: "",
		albedo_tex: matNullTexTemp,
		metallicgloss_tex: matNullTexTemp,
		normal_tex: matNullTexTemp,
		height_tex: matNullTexTemp
	};

	// write texture first
	if (editor.hasTextureChannel("albedo")) {
		const albedoGuid = newGuid();
		zip.file(
			albedoGuid + "/asset",
			canvasToBase64(editor.getChannelCanvasImage("albedo").canvas),
			{ base64: true }
		);
		zip.file(
			albedoGuid + "/asset.meta",
			template(textureMetaTemplate, {
				texture_guid: albedoGuid,
				type: TextureType.Default
			})
		);
		zip.file(
			albedoGuid + "/pathname",
			"Assets/" + materialName + "/Albedo.png"
		);

		matData.albedo_tex = template(matTexTemp, {
			guid: albedoGuid,
			repeatx: 1,
			repeaty: 1
		});
	}

	if (editor.hasTextureChannel("normal")) {
		const guid = newGuid();

		const normalCanvas = editor.getChannelCanvasImage("normal");
		exporter.canvas.width = normalCanvas.width();
		exporter.canvas.height = normalCanvas.height();

		fixNormalMap(exporter, normalCanvas.createTexture(exporter.gl));
		zip.file(guid + "/asset", canvasToBase64(exporter.canvas), {
			base64: true
		});
		zip.file(
			guid + "/asset.meta",
			template(textureMetaTemplate, {
				texture_guid: guid,
				type: TextureType.NormalMap
			})
		);
		zip.file(guid + "/pathname", "Assets/" + materialName + "/Normal.png");

		matData.normal_tex = template(matTexTemp, {
			guid: guid,
			repeatx: 1,
			repeaty: 1
		});
	}

	if (
		editor.hasTextureChannel("metalness") ||
		editor.hasTextureChannel("roughness")
	) {
		const guid = newGuid();

		//var normalCanvas = viewer.normalCanvas;
		// resize canvas
		// todo: figure out how to derive canvas size
		exporter.canvas.width = editor.getImageWidth();
		exporter.canvas.height = editor.getImageHeight();

		let mTex = null;
		if (editor.hasTextureChannel("metalness"))
			mTex = editor
				.getChannelCanvasImage("metalness")
				.createTexture(exporter.gl);

		let rTex = null;
		if (editor.hasTextureChannel("roughness"))
			rTex = editor
				.getChannelCanvasImage("roughness")
				.createTexture(exporter.gl);

		generateMetallicGloss(exporter, mTex, rTex);
		zip.file(guid + "/asset", canvasToBase64(exporter.canvas), {
			base64: true
		});
		zip.file(
			guid + "/asset.meta",
			template(textureMetaTemplate, {
				texture_guid: guid,
				type: TextureType.Default
			})
		);
		zip.file(
			guid + "/pathname",
			"Assets/" + materialName + "/MetallicGloss.png"
		);

		matData.metallicgloss_tex = template(matTexTemp, {
			guid: guid,
			repeatx: 1,
			repeaty: 1
		});
	}

	// NOTE: Unity has the metallic and glossy fields in one texture (glossiness being the alpha channel)
	// glosiness is the inverse of the roughness map

	// write material last
	const matGuid = newGuid();
	/*
        zip.file(matGuid+"/asset",template(materialTemplate,{
            keywords:"",
            albedo_guid:albedoGuid
        }));
        */
	zip.file(matGuid + "/asset", template(materialTemplate, matData));
	zip.file(
		matGuid + "/asset.meta",
		template(materialMetaTemplate, { material_guid: matGuid })
	);
	zip.file(
		matGuid + "/pathname",
		"Assets/" + materialName + "/" + materialName + ".mat"
	);
	//zip.file(matGuid+"/asset","");//preview
	zip.file(matGuid + "/asset", template(materialTemplate, matData));
	zip.file(
		matGuid + "/asset.meta",
		template(materialMetaTemplate, { material_guid: matGuid })
	);
	zip.file(
		matGuid + "/pathname",
		"Assets/" + materialName + "/" + materialName + ".mat"
	);

	return zip.generateAsync({ type: "nodebuffer" });
}

function newGuid() {
	return "xxxxxxxxxxxx4xxxyxxxxxxxxxxxxxxx".replace(/[xy]/g, function(c) {
		const r = (Math.random() * 16) | 0,
			v = c == "x" ? r : (r & 0x3) | 0x8;
		return v.toString(16);
	});
}

function template(str, data) {
	let res = str;
	for (const name in data) {
		res = res.replace("{{" + name + "}}", data[name]);
	}

	return res;
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
	const canvas = document.createElement("canvas");
	const gl = canvas.getContext("webgl");

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

function compileShader(gl, source, shaderType) {
	const shader = gl.createShader(shaderType);

	gl.shaderSource(shader, source);
	gl.compileShader(shader);

	if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
		alert(gl.getShaderInfoLog(shader));
		return null;
	}

	return shader;
}

function buildShaderProgram(gl, vertSource, fragSource) {
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

	return shaderProgram;
}

//var texCoordBuffer = new WebGLBuffer;
//var posBuffer = new WebGLBuffer;

// render quad using shader and texture inputs
// returns HtmlImageElement
function renderToImage(exporter, program, inputs) {
	const gl = exporter.gl;

	gl.viewport(0, 0, exporter.canvas.width, exporter.canvas.height);

	// bind texture to fbo
	//gl.clearColor(0,0,1,1);
	gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

	// bind shader
	gl.useProgram(program);

	// pass textures
	let texIndex = 0;
	for (const i in inputs) {
		const input = inputs[i];
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
	const posLoc = gl.getAttribLocation(program, "a_pos");
	const texCoordLoc = gl.getAttribLocation(program, "a_texCoord");

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
	const gl = exporter.gl;

	// provide texture coordinates for the rectangle.
	const texCoordBuffer = gl.createBuffer();
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

	const posBuffer = gl.createBuffer();
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
	let data = canvas.toDataURL();
	// todo: maybe script header?
	// https://code-examples.net/en/q/6f412f
	data = data.replace(/^data:image\/(png|jpg);base64,/, "");
	return data;
}

// todo: cleanup textures!!

// initialize
initGLAndResources(exporter);
