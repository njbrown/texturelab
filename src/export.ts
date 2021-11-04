import jimp from "jimp";
import sharp from "sharp";
import { Editor } from "./lib/editor";
import { BitDepth, encode } from "fast-png";
import fs from "fs";
import { PNG } from "pngjs";
import {
	Float16Array,
	isFloat16Array,
	getFloat16,
	setFloat16,
	hfround
} from "@petamoriken/float16";
import {
	TextureComponents,
	TextureDataConverter
} from "./lib/designer/texturedataconverter";
import { TextureDataType } from "./lib/designer/gl";
import { DesignerNode } from "./lib/designer/designernode";
import { Designer } from "./lib/designer";
import AdmZip from "adm-zip";
import path from "path";
import electron from "electron";

export enum ImageFileType {
	Png = "png",
	Jpg = "jpg",
	Tga = "tga",
	Tif = "tif"
}

export enum ColorSpace {
	Linear = "linear",
	sRGB = "srgb"
}

export enum OutputType {
	Folder,
	Zip,
	UnityPackage
}

export enum TextureTransform {
	InvertX,
	InvertY,
	InvertZ,
	LinearizeRoughness
}

export class ExportTextureSettings {
	channelName: string = ""; // albedo, roughness, normal, etc..
	colorSpace: ColorSpace = ColorSpace.Linear;
	fileType: ImageFileType = ImageFileType.Png;
	transforms: TextureTransform[] = [];
}

// can have multiple settings and presets
export class ExportSettings {
	name: string = "";
	filePattern: string = "{project}_{file}";
	// textureSettings: Map<string, ExportTextureSettings> = new Map<
	// 	string,
	// 	ExportTextureSettings
	// >();
	// enabled: boolean = true;
	outputType: OutputType = OutputType.Zip;
	outputPath: string = "";
}

export class Exporter {
	async export(editor: Editor, settings: ExportSettings) {
		const designer = editor.designer;
		const conv = new TextureDataConverter(editor.designer.gl);
		// sample config

		// assume the settings have all the nodes listen already
		// settings is the source of truth
		console.log(editor.textureChannels);

		const exportNodes = this.gatherExportNodes(designer, editor);
		console.log(exportNodes);

		let files: Map<string, ArrayBuffer> = new Map<string, ArrayBuffer>();

		for (const node of exportNodes) {
			const channelNode = node.node;

			// const pixelData = channelNode.getPixelData();
			// const pixelData = conv.getData(
			// 	channelNode.node.tex,
			// 	designer.width,
			// 	designer.height,
			// 	TextureDataType.Uint16,
			// 	TextureComponents.RGBA,
			// 	true
			// );

			const pixelData = conv.getData(
				channelNode.tex,
				designer.width,
				designer.height,
				node.dataType,
				node.components,
				true
			);

			// too expensive
			// convertRange(pixelData);

			// console.log(pixelData);
			// console.log(floatData);
			// for (let i = 0; i < 20; i++) {
			// 	console.log(floatData[i]);
			// }

			// const exportPath =
			// 	"C:/Users/Nicolas Brown/Desktop/export-" + node.name + ".png";
			// console.log("exporting to: " + exportPath);

			const fileData = exportNodeAsPng(
				node,
				pixelData,
				designer.width,
				designer.height
			);

			const projectName = settings.name
				.replace(" ", "_")
				.replace(/[^0-9a-zA-Z_]/g, "");

			const fileName = interpolateString(settings.filePattern, {
				project: projectName,
				name: node.name
			});

			// files.set(node.name + ".png", fileData);
			files.set(fileName + ".png", fileData);

			// write to file if folder mode
			// fs.writeFile(exportPath, bytes, function(err) {
			// 	if (err) alert("Error exporting texture: " + err);
			// });

			// write to zip otherwise

			// const imgCanvas = editor.getChannelCanvasImage(channelName);
			// if (imgCanvas) {
			// 	const imgData = imgCanvas.canvas.toDataURL("image/png");

			// 	//todo: get raw image data from gpu in floating point
			// }
		}

		if (settings.outputType == OutputType.Folder) {
			exportFilesToFolder(files, settings.outputPath);
		} else {
			exportFilesToZip(files, settings.outputPath);
			electron.remote.shell.showItemInFolder(settings.outputPath);
		}

		// export them

		// const image = await jimp.read("");
	}

	calculateFileName(fileName: string, project: string): string {
		return null;
	}

	// returns all nodes ready for export with their data
	// todo: use display channel if no name property specified
	gatherExportNodes(designer: Designer, editor: Editor) {
		// gather all export nodes
		// let exportNodes = [];

		let nodes = designer.nodes.filter(x => x.typeName === "output");

		let exportNodes: ExportNode[] = [];

		for (const node of nodes) {
			let exportNode = new ExportNode();

			exportNode.node = node;
			for (const prop of node.properties) {
				if (prop.name === "name") {
					exportNode.name = prop.getValue();
				}
				if (prop.name === "precision") {
					const precision = prop.getValue() as number;

					// 0 -> 8bits
					// 1 -> 16bits
					if (precision === 0) exportNode.dataType = TextureDataType.Uint8;
					if (precision === 1) exportNode.dataType = TextureDataType.Uint16;
				}
				if (prop.name === "components") {
					const components = prop.getValue() as number;

					// 0 -> RGBA
					// 1 -> RGB
					// 2 -> R
					// 3 -> G
					// 4 -> B
					// 5 -> A
					if (components === 0) exportNode.components = TextureComponents.RGBA;
					if (components === 1) exportNode.components = TextureComponents.RGB;
					if (components === 2) exportNode.components = TextureComponents.R;
					if (components === 3) exportNode.components = TextureComponents.G;
					if (components === 4) exportNode.components = TextureComponents.B;
					if (components === 5) exportNode.components = TextureComponents.A;
				}
			}

			if (!exportNode.name || exportNode.name === "") {
				exportNode.name = getChannelNameForNode(exportNode.node, editor);
			}

			exportNodes.push(exportNode);
		}

		return exportNodes;
	}
}

function getChannelNameForNode(node: DesignerNode, editor: Editor) {
	const channels = editor.textureChannels;
	// slow.
	// create dictionary with nodeId as key instead
	for (const key of channels.keys()) {
		if (channels.get(key) === node) return key;
	}

	return "";
}

function exportNodeAsPng(
	node: ExportNode,
	pixelData: ArrayBuffer,
	width: number,
	height: number
): ArrayBuffer {
	try {
		let bitDepth: BitDepth = 8;
		if (node.dataType === TextureDataType.Uint16) bitDepth = 16;

		let channels: number = 4;
		// if (node.components === TextureComponents.RGBA) channels = 4;
		if (node.components === TextureComponents.RGB) channels = 3;
		if (
			node.components === TextureComponents.R ||
			node.components === TextureComponents.G ||
			node.components === TextureComponents.B ||
			node.components === TextureComponents.A
		)
			channels = 1;

		// FAST-PNG
		const bytes = encode({
			data:
				bitDepth == 8 ? new Uint8Array(pixelData) : new Uint16Array(pixelData),
			width: width,
			height: height,
			depth: bitDepth,
			channels: channels
		});

		return bytes.buffer;

		// fs.writeFile(exportPath, bytes, function(err) {
		// 	if (err) alert("Error exporting texture: " + err);
		// });

		// await sharp(pixelData, {
		// 	raw: {
		// 		width: editor.designer.width,
		// 		height: editor.designer.height,
		// 		channels: 4
		// 	}
		// })
		// 	// .pipelineColourspace("rgb")
		// 	// .toColorspace("rgb")
		// 	.png({})
		// 	.toFile(exportPath);

		// PNGJS
		// https://github.com/lukeapage/pngjs/blob/master/examples/16bit_write.js
		// let png = new PNG({
		// 	width: editor.designer.width,
		// 	height: editor.designer.height,
		// 	inputColorType: 6,
		// 	colorType: 6,
		// 	bitDepth: 16,
		// 	inputHasAlpha: true
		// });

		// png.data = Buffer.from(pixelData);
		// png.gamma = 1;
		// png.pack().pipe(fs.createWriteStream(exportPath));
	} catch (error) {
		console.log("error saving file");
		console.log(error);
	}
}

class ExportNode {
	node: DesignerNode;
	components: TextureComponents = TextureComponents.RGBA;
	dataType: TextureDataType = TextureDataType.Uint8;
	name: string = "";
}

function exportFilesToFolder(
	files: Map<string, ArrayBuffer>,
	folderPath: string
) {
	for (let fileName of files.keys()) {
		console.log(fileName);
		const exportPath = path.join(folderPath, fileName);
		const bytes = files.get(fileName);

		fs.writeFile(exportPath, new Uint8Array(bytes), function(err) {
			if (err) alert("Error exporting texture: " + err);
		});
	}
}
function exportFilesToZip(
	files: Map<string, ArrayBuffer>,
	zipFilePath: string
) {
	const zip = new AdmZip();

	for (let fileName of files.keys()) {
		const bytes = files.get(fileName);
		console.log(bytes);
		zip.addFile(fileName, Buffer.from(bytes));
	}

	zip.writeZip(zipFilePath);
}

function convertRange(data: Uint16Array) {
	const UINT_MAX = 65535.0;
	const VALUE_MAX = 15360.0;

	const floatData = new Float16Array(data.buffer);
	for (let i = 0; i < data.length; i++) {
		// if (i < 10) console.log(floatData[i] * UINT_MAX);
		data[i] = floatData[i] * UINT_MAX;
	}
}

function interpolateString(text: string, exportData: object) {
	if (!text) return "";

	// run regex on text and replace values from the string
	text = text.replace(
		/\${([a-z]+)}/g,
		(wholeMatch: string, path: any): string => {
			// console.log(path);
			const data = exportData[path];
			if (!data) return "";

			return data;
		}
	);

	// console.log(text);

	return text;
}

// class PixelDataConverter
// {
// 	public static rgbaTo
// }
