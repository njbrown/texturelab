import jimp from "jimp";
import sharp from "sharp";
import { Editor } from "./lib/editor";
import { encode } from "fast-png";
import fs from "fs";
import { PNG } from "pngjs";
import {
	Float16Array,
	isFloat16Array,
	getFloat16,
	setFloat16,
	hfround
} from "@petamoriken/float16";

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
	filePattern: string = "${channel}.${fileType}";
	textureSettings: Map<string, ExportTextureSettings> = new Map<
		string,
		ExportTextureSettings
	>();
	enabled: boolean = true;
	outputType: OutputType = OutputType.Zip;
	outputPath: string = "";
}

export class Exporter {
	async export(editor: Editor, settings: ExportSettings) {
		// sample config

		// assume the settings have all the nodes listen already
		// settings is the source of truth
		console.log(editor.textureChannels);

		for (const channelName of settings.textureSettings.keys()) {
			if (!editor.textureChannels.has(channelName)) continue;

			const channelNode = editor.textureChannels.get(channelName);
			const pixelData = channelNode.getPixelData();

			convertRange(pixelData);
			// console.log(pixelData);
			// console.log(floatData);
			// for (let i = 0; i < 20; i++) {
			// 	console.log(floatData[i]);
			// }

			const exportPath =
				"C:/Users/Nicolas Brown/Desktop/export-" + channelName + ".png";
			console.log("exporting to: " + exportPath);

			try {
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

				// FAST-PNG
				const bytes = encode({
					data: pixelData,
					width: editor.designer.width,
					height: editor.designer.height,
					depth: 16,
					channels: 4
				});

				fs.writeFile(exportPath, bytes, function(err) {
					if (err) alert("Error exporting texture: " + err);
				});

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

			// const imgCanvas = editor.getChannelCanvasImage(channelName);
			// if (imgCanvas) {
			// 	const imgData = imgCanvas.canvas.toDataURL("image/png");

			// 	//todo: get raw image data from gpu in floating point
			// }
		}

		// const image = await jimp.read("");
	}
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
