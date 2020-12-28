import { NodeRenderContext } from "@/lib/designer";
import { FloatProperty } from "@/lib/designer/properties";
import { DesignerNode } from "../../designer/designernode";

const VALUE_MAX = 15360.0;
const ONE_OVER_VALUE_MAX = 1.0 / VALUE_MAX;

// https://xjavascript.com/view/639466/read-pixels-in-webgltexture-rendering-webgl-to-texture
// NOTE: THIS CREATES A HALF FLOAT TEXTURE, NOT UNSIGNED BYTE
// more accuracy is needed for the output
export class FloodFill extends DesignerNode {
	// working pixels
	pixels: Uint16Array;
	readFbo: WebGLFramebuffer;
	gen: FloodFillGenerator;

	public init() {
		this.title = "Flood Fill";

		this.addInput("image");

		const width = this.designer.width;
		const height = this.designer.height;

		this.pixels = new Uint16Array(width * height * 4);

		// create framebuffer for reading pixels from input texture
		this.readFbo = this.gl.createFramebuffer();

		this.gen = new FloodFillGenerator();
		this.gen.init(width, height);
	}

	public render(context: NodeRenderContext) {
		const inputs = context.inputs;

		if (inputs.length == 0) return;

		let inputTexture = inputs[0].node.tex;

		const width = this.designer.width;
		const height = this.designer.height;

		let gridSize = width * height;
		let gl = context.gl as WebGL2RenderingContext;
		var pixels = this.pixels;

		gl.bindFramebuffer(gl.FRAMEBUFFER, this.readFbo);
		gl.framebufferTexture2D(
			gl.FRAMEBUFFER,
			gl.COLOR_ATTACHMENT0,
			gl.TEXTURE_2D,
			inputTexture,
			0
		);
		if (gl.checkFramebufferStatus(gl.FRAMEBUFFER) == gl.FRAMEBUFFER_COMPLETE) {
			gl.readPixels(0, 0, width, height, gl.RGBA, gl.HALF_FLOAT, pixels);
		} else {
			alert("Bevel: unable to read from FBO");
		}
		gl.bindFramebuffer(gl.FRAMEBUFFER, null);

		this.gen.process(pixels);

		gl.bindTexture(gl.TEXTURE_2D, this.tex);

		// https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glTexImage2D.xhtml
		const level = 0;
		const internalFormat = gl.RGBA16F;
		const border = 0;
		const format = gl.RGBA;
		const type = gl.FLOAT;
		// const data = pixels;
		gl.texImage2D(
			gl.TEXTURE_2D,
			level,
			internalFormat,
			this.designer.width,
			this.designer.height,
			border,
			format,
			type,
			this.gen.results
		);

		// set the filtering so we don't need mips
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);

		gl.bindTexture(gl.TEXTURE_2D, null);

		console.log(this.gen.results);
	}
}

class Vector {
	x: number = 0;
	y: number = 0;

	constructor(x: number, y: number) {
		this.x = x;
		this.y = y;
	}
}

class FloodFillPixel {
	localX: number = 0;
	localY: number = 0;

	globalX: number = 0;
	globalY: number = 0;

	constructor(
		localX: number,
		localY: number,
		globalX: number,
		globalY: number
	) {
		this.localX = localX;
		this.localY = localY;

		this.globalX = globalX;
		this.globalY = globalY;
	}
}

class Box {
	left: number = 0;
	top: number = 0;
	right: number = 0;
	bottom: number = 0;

	// coordinates of captured pixels
	pixels: FloodFillPixel[] = [];

	public expand(x: number, y: number) {
		//console.log(this);
		this.left = Math.min(this.left, x);
		this.top = Math.min(this.top, y);

		this.right = Math.max(this.right, x);
		this.bottom = Math.max(this.bottom, y);

		//console.log(x + " , " + y);
	}

	public negativeInfinity() {
		this.left = Number.POSITIVE_INFINITY;
		this.top = Number.POSITIVE_INFINITY;

		this.right = Number.NEGATIVE_INFINITY;
		this.bottom = Number.NEGATIVE_INFINITY;
	}

	get width() {
		return this.right - this.left;
	}

	get height() {
		return this.bottom - this.top;
	}

	get x() {
		return this.left;
	}

	get y() {
		return this.top;
	}
}

class FloodFillGenerator {
	rects: Box[] = [];

	visited: Array<boolean> = [];
	width: number;
	height: number;
	results: Float32Array;

	constructor() {}

	init(width: number, height: number) {
		this.width = width;
		this.height = height;

		let visited = this.visited;
		visited.length = width * height;

		this.results = new Float32Array(width * height * 4);
	}

	process(pixels: Uint16Array) {
		let threshold = 0.1;

		// allocate image of same size to track visited pixels
		let visited = this.visited;
		for (let i = 0; i < visited.length; i++) visited[i] = false;

		let width = this.width;
		let height = this.height;

		this.rects = [];
		for (let y = 0; y < height; y++) {
			for (let x = 0; x < width; x++) {
				if (visited[y * width + x] == true) continue;

				this.captureIsland(x, y, pixels, width, height, visited);
			}
		}

		this.renderOutput(this.results);
	}

	captureIsland(
		x: number,
		y: number,
		pixels: Uint16Array,
		width: number,
		height: number,
		visited: Array<boolean>
	) {
		let rect: Box = new Box();
		rect.negativeInfinity();

		// this queue contains coords in a global space
		// i.e. values can be negative or larger than the
		// texture size
		let queue: Vector[] = [new Vector(x, y)];
		while (queue.length > 0) {
			let globalPixel = queue.shift();
			// should never happen
			if (!globalPixel) continue;

			let pixel = this.mapPixelToLocal(globalPixel, width, height);

			// skip if already visited
			if (visited[pixel.y * width + pixel.x] == true) continue;

			visited[pixel.y * width + pixel.x] = true;

			// 0.1 or less denotes a dark pixel
			// skip this
			let intensity = this.getIntensity(
				pixel.x,
				pixel.y,
				pixels,
				width,
				height
			);
			if (intensity < 0.1) {
				continue;
			}

			// ok it's a white enough pixel, add it's neighbors to the queue
			// and expand the rect
			rect.expand(globalPixel.x, globalPixel.y);
			rect.pixels.push(
				new FloodFillPixel(pixel.x, pixel.y, globalPixel.x, globalPixel.y)
			);

			queue.push(new Vector(globalPixel.x + 1, globalPixel.y + 0));
			queue.push(new Vector(globalPixel.x - 1, globalPixel.y + 0));
			queue.push(new Vector(globalPixel.x + 0, globalPixel.y + 1));
			queue.push(new Vector(globalPixel.x + 0, globalPixel.y - 1));
		}

		if (rect.width <= 0 || rect.height <= 0) return;

		this.rects.push(rect);
	}

	mapPixelToLocal(globalPixel: Vector, width: number, height: number): Vector {
		return new Vector(
			wrapAround(globalPixel.x, width),
			wrapAround(globalPixel.y, height)
		);
	}

	getIntensity(
		x: number,
		y: number,
		data: Uint16Array,
		width: number,
		height: number
	): number {
		let col = 0;
		col += data[4 * (width * y + x) + 0];
		col += data[4 * (width * y + x) + 1];
		col += data[4 * (width * y + x) + 2];

		//						average    map to 0.0 - 1.0
		let intensity = col * (1.0 / 3.0) * ONE_OVER_VALUE_MAX;

		return intensity;
	}

	renderRects(canvas: HTMLCanvasElement) {
		let ctx = canvas.getContext("2d");
		if (!ctx) return;

		ctx.strokeStyle = "red";
		ctx.lineWidth = 1;

		for (let rect of this.rects) {
			ctx.strokeRect(rect.x, rect.y, rect.width, rect.height);
		}
	}

	renderOutput(results: Float32Array) {
		let width = this.width;
		let height = this.height;

		// clear results first
		for (let i = 0; i < results.length; i++) results[i] = 0;

		for (let rect of this.rects) {
			let sx = rect.width * (1.0 / width); // * 255;
			let sy = rect.height * (1.0 / height); // * 255;

			for (let pixel of rect.pixels) {
				let u = (pixel.globalX - rect.left) / rect.width; // * 255;
				let v = (pixel.globalY - rect.top) / rect.height; // * 255;

				setColorAtPixel(
					results,
					width,
					height,
					pixel.localX,
					pixel.localY,
					u,
					v,
					sx,
					sy
				);
				//setColorAtPixel(data, pixel.x, pixel.y, 255, 0, 0, 255);
			}
		}
	}
}

// https://stackoverflow.com/questions/14325750/using-mod-to-wrap-around
// upperBound is never touchec
function wrapAround(value: number, upperBound: number) {
	return (value + upperBound - 1) % upperBound;
}

// https://codepen.io/Geeyoam/pen/vLGZzG
function getColorAtPixel(imageData: ImageData, x: number, y: number) {
	const { width, data } = imageData;

	return {
		r: data[4 * (width * y + x) + 0],
		g: data[4 * (width * y + x) + 1],
		b: data[4 * (width * y + x) + 2],
		a: data[4 * (width * y + x) + 3]
	};
}

function setColorAtPixel(
	data: Float32Array,
	width: number,
	height: number,
	x: number,
	y: number,
	r: number,
	g: number,
	b: number,
	a: number
) {
	data[4 * (width * y + x) + 0] = r;
	data[4 * (width * y + x) + 1] = g;
	data[4 * (width * y + x) + 2] = b;
	data[4 * (width * y + x) + 3] = a;
}
