const IMAGE_RENDER_SIZE = 1000;

function _getMousePos(canvas, evt) {
	const rect = canvas.getBoundingClientRect();
	return new Vector2(evt.clientX - rect.left, evt.clientY - rect.top);
}

class Vector2 {
	x: number;
	y: number;

	constructor(x: number, y: number) {
		this.x = x;
		this.y = y;
	}
}

export enum DrawMode {
	Single,
	Nine
}

class Rect {
	protected visible = true;

	protected x = 0;
	protected y = 0;
	protected width: number;
	protected height: number;

	color: string;

	public constructor() {
		//this.scene = scene;
		//scene.addItem(this);
		this.width = 1;
		this.height = 1;
		this.color = "rgb(255, 50, 50)";
	}

	public setSize(w: number, h: number) {
		this.width = w;
		this.height = h;
	}

	public isPointInside(px: number, py: number): boolean {
		if (
			px >= this.x &&
			px <= this.x + this.width &&
			py >= this.y &&
			py <= this.y + this.height
		)
			return true;
		return false;
	}

	public setCenter(x: number, y: number) {
		this.x = x - this.width / 2;
		this.y = y - this.height / 2;
	}

	public centerX(): number {
		return this.x + this.width / 2;
	}

	public centerY(): number {
		return this.y + this.height / 2;
	}

	public move(dx: number, dy: number) {
		this.x += dx;
		this.y += dy;
	}

	// to be overriden
	draw(ctx: CanvasRenderingContext2D) {
		// background
		ctx.beginPath();
		ctx.fillStyle = this.color;
		ctx.rect(this.x, this.y, this.width, this.height);
		ctx.fill();

		// border
		ctx.beginPath();
		ctx.lineWidth = 4;
		ctx.strokeStyle = "rgb(0, 0, 0)";
		ctx.rect(this.x, this.y, this.width, this.height);
		ctx.stroke();
	}
}

export class DragZoom {
	canvas: HTMLCanvasElement;
	context: CanvasRenderingContext2D;

	mousePos: Vector2;
	prevMousePos: Vector2;
	mouseDownPos: Vector2; // pos of last mouse down
	mouseDragDiff: Vector2; // mouse drag diff

	zoomFactor: number;
	offset: Vector2;

	panning: boolean;
	panStart: SVGPoint;

	rect: Rect;
	image: HTMLCanvasElement;

	public drawMode: DrawMode;

	constructor(canvas: HTMLCanvasElement) {
		this.canvas = canvas;
		this.context = this.canvas.getContext("2d");
		this.image = null;

		const self = this;
		canvas.addEventListener("mousemove", function(evt: MouseEvent) {
			self.onMouseMove(evt);
		});
		canvas.addEventListener("mousedown", function(evt: MouseEvent) {
			self.onMouseDown(evt);
		});
		canvas.addEventListener("mouseup", function(evt: MouseEvent) {
			self.onMouseUp(evt);
		});
		canvas.addEventListener("mouseout", function(evt: MouseEvent) {
			self.onMouseOut(evt);
		});
		canvas.addEventListener("mousewheel", function(evt: WheelEvent) {
			self.onMouseScroll(evt);
		});
		canvas.addEventListener("contextmenu", function(evt: MouseEvent) {
			evt.preventDefault();
		});
		canvas.addEventListener("resize", function(evt: MouseEvent) {
			console.log("2d canvas resized");
		});

		this.drawMode = DrawMode.Single;

		// this.zoomFactor = 1;
		this.zoomFactor = 0.4;
		this.offset = new Vector2(0, 0);

		this.mousePos = new Vector2(0, 0);

		this.rect = new Rect();
		this.rect.setSize(50, 50);

		// offset to put center(0,0) in middle of view
		this.offset = new Vector2(canvas.width * 0.5, canvas.height * 0.5);
	}

	onResize(width: number, height: number) {
		this.offset = new Vector2(width * 0.5, height * 0.5);
	}

	// puts image in center and set appropriate zoom level
	centerImage() {
		this.offset = new Vector2(
			this.canvas.width * 0.5,
			this.canvas.height * 0.5
		);
		this.zoomFactor = 0.4;
	}

	setImage(image: HTMLCanvasElement) {
		this.image = image;

		// center image in view
	}

	getAbsPos() {
		return new Vector2(this.canvas.offsetLeft, this.canvas.offsetTop);
	}

	onMouseDown(evt: MouseEvent) {
		if (evt.button == 1 || evt.button == 2) {
			this.panning = true;

			this.mouseDownPos = _getMousePos(this.canvas, evt);
		}

		this.mousePos = _getMousePos(this.canvas, evt);
	}

	onMouseUp(evt: MouseEvent) {
		// this.mouseX = pos.x;
		// this.mouseY = pos.y;
		if (evt.button == 1 || evt.button == 2) {
			this.panning = false;
		}
	}

	onMouseMove(evt: MouseEvent) {
		this.prevMousePos = this.mousePos;
		this.mousePos = _getMousePos(this.canvas, evt);

		if (this.panning) {
			const prev = this.canvasToScene(this.prevMousePos);
			const cur = this.canvasToScene(this.mousePos);
			const diff = new Vector2(prev.x - cur.x, prev.y - cur.y);
			// const diff = new Vector2(this.prevMousePos.x - this.mousePos.x, this.prevMousePos.y - this.mousePos.y);

			const factor = this.zoomFactor;
			this.offset.x -= diff.x * factor;
			this.offset.y -= diff.y * factor;
		}

		// var lastX = this.mouseX;
		// var lastY = this.mouseY;
		// var pos = this.getScenePos(evt);
		// this.mouseX = pos.x;
		// this.mouseY = pos.y;

		// if (this.panning) {
		//     // convert to scene space first
		//     //var lastPt = this.contextExtra.transformedPoint(lastX, lastY);
		//     //var pt = this.contextExtra.transformedPoint(this.mouseX, this.mouseY);
		//     //this.context.translate(pt.x - lastPt.x, pt.y - lastPt.y);
		//     //console.log(pt.x - this.panStart.x, pt.y - this.panStart.y);
		//     console.log(this.mouseX - this.panStart.x, this.mouseY - this.panStart.y);
		//     this.context.translate(this.mouseX - this.panStart.x, this.mouseY - this.panStart.y);
		//     //this.panStart = pos;
		// }
	}

	onMouseScroll(evt: WheelEvent) {
		// no panning while zooming
		if (this.panning) return;

		const pos = _getMousePos(this.canvas, evt);
		const delta = (<any>evt).wheelDelta > 0 ? 1.1 : 1.0 / 1.1;

		// offset from mouse pos
		// find offset from previous zoom then move offset by that value

		this.zoomFactor *= delta;
		this.offset.x = pos.x - (pos.x - this.offset.x) * delta; // * (factor);
		this.offset.y = pos.y - (pos.y - this.offset.y) * delta; // * (factor);

		//this.zoom(pos.x, pos.y, delta);

		evt.preventDefault();
		return false;
	}

	onMouseOut(evt: MouseEvent) {
		// cancel panning
		this.panning = false;
	}

	zoom(x: number, y: number, level: number) {
		// var scaleFactor = 1.01;
		// var pt = this.contextExtra.transformedPoint(x,y);
		// var factor = Math.pow(scaleFactor, level);
	}

	draw() {
		const ctx = this.context;

		ctx.setTransform(1, 0, 0, 1, 0, 0);
		ctx.fillStyle = "rgb(50,50,50)";
		ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

		//ctx.scale(this.zoomFactor, this.zoomFactor);
		//ctx.translate(this.offset.x, this.offset.y);

		ctx.setTransform(
			this.zoomFactor,
			0,
			0,
			this.zoomFactor,
			this.offset.x,
			this.offset.y
		);

		// ctx.translate(this.offset.x, this.offset.y);
		// ctx.scale(this.zoomFactor, this.zoomFactor);

		// highlight rect if mouse over
		const scenePos = this.canvasToScene(this.mousePos);
		//console.log(scenePos);
		if (this.rect.isPointInside(scenePos.x, scenePos.y)) {
			this.rect.color = "rgb(0, 255, 255)";
		} else {
			this.rect.color = "rgb(255, 50, 50)";
		}

		if (this.image) {
			if (this.drawMode == DrawMode.Single) {
				this.drawImage(0, 0);
			} else {
				// top
				this.drawImage(-1, -1);
				this.drawImage(0, -1);
				this.drawImage(1, -1);

				// middle
				this.drawImage(-1, 0);
				this.drawImage(0, 0);
				this.drawImage(1, 0);

				// bottom
				this.drawImage(-1, 1);
				this.drawImage(0, 1);
				this.drawImage(1, 1);
			}

			//   this.context.lineWidth = 5;
			//   this.context.strokeRect(
			//     -IMAGE_RENDER_SIZE * 0.5,
			//     -IMAGE_RENDER_SIZE * 0.5,
			//     IMAGE_RENDER_SIZE,
			//     IMAGE_RENDER_SIZE
			//   );
		}
	}

	drawImage(offsetX: number, offsetY: number) {
		this.context.drawImage(
			this.image,
			-IMAGE_RENDER_SIZE * 0.5 + offsetX * IMAGE_RENDER_SIZE,
			-IMAGE_RENDER_SIZE * 0.5 + offsetY * IMAGE_RENDER_SIZE,
			IMAGE_RENDER_SIZE,
			IMAGE_RENDER_SIZE
		);
	}

	// converts from canvas(screen) coords to the scene(world) coords
	canvasToScene(pos: Vector2): Vector2 {
		//return new Vector2(pos.x * (1.0 / this.zoomFactor) - this.offset.x, pos.y * (1.0 / this.zoomFactor) - this.offset.y);
		return new Vector2(
			(pos.x - this.offset.x) * (1.0 / this.zoomFactor),
			(pos.y - this.offset.y) * (1.0 / this.zoomFactor)
		);
		//return new Vector2(pos.x *  this.zoomFactor - this.offset.x, pos.y * this.zoomFactor - this.offset.y);
	}
}
