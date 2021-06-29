import { NativeImage } from "electron";

enum ImageSource {
	Inline = "inline", // in .texture file
	Embedded = "embedded" // in .texlab package as a seperate file
}

export class Image {
	canvas: HTMLCanvasElement;
	path: string;
	src: string;
	type: ImageSource;

	constructor(
		path: string,
		canvasSource: CanvasImageSource,
		width: number,
		height: number
	) {
		this.init(path, canvasSource, width, height);
	}

	private init(
		path: string,
		canvasSource: CanvasImageSource,
		width: number,
		height: number
	) {
		if (canvasSource != null) {
			let canvas = document.createElement("canvas") as HTMLCanvasElement;
			canvas.width = width;
			canvas.height = height;

			let ctx = canvas.getContext("2d");
			ctx.drawImage(canvasSource, 0, 0);

			this.canvas = canvas;
		}

		this.path = path;
	}

	get isEmpty() {
		return this.canvas == null;
	}

	static empty(): Image {
		return new Image(null, null, 0, 0);
	}

	static load(path: string): Image {
		let nativeImage = NativeImage.createFromPath(path);
		let img: HTMLImageElement = document.createElement(
			"image"
		) as HTMLImageElement;

		img.src = nativeImage.toDataURL();

		return new Image(path, img, img.width, img.height);
	}

	clone(): Image {
		return new Image(this.path, this.canvas, this.width, this.height);
	}

	get width() {
		return this.canvas.width;
	}

	get height() {
		return this.canvas.height;
	}

	serialize() {
		return {
			type: ImageSource.Inline,
			src: this.canvas.toDataURL("image/png")
		};
	}

	// handle async
	deserialize(obj: any, completeCallback: () => void = null) {
		console.log(obj);

		// note: only works with inline images right now
		if (obj.type !== ImageSource.Inline) return;

		let img: HTMLImageElement = document.createElement(
			"img"
		) as HTMLImageElement;

		img.onload = () => {
			console.log("image loaded");
			console.log(img);
			console.log(img.width);
			console.log(img.height);

			// // flip image
			// let canvas = document.createElement("canvas");
			// canvas.width = img.width;
			// canvas.height = img.height;

			// let ctx = canvas.getContext("2d");
			// ctx.save();
			// ctx.clearRect(0, 0, canvas.width, canvas.height);
			// //ctx.translate(0, img.height);
			// //ctx.scale(1, -1);
			// ctx.drawImage(img, 0, 0, img.width, img.height);
			// ctx.restore();

			this.init("", img, img.width, img.height);

			completeCallback();
		};

		img.src = obj.src;
	}
}
