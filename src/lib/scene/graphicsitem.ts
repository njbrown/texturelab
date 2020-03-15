import { NodeScene } from "../scene";

export class MouseEvent {
	// scene space
	globalX: number;
	globalY: number;

	localX: number;
	localY: number;

	mouseButton: number;

	// default is accepted
	private accepted: boolean = true;
	public accept() {
		this.accepted = true;
	}

	public reject() {
		this.accepted = false;
	}
}

export class MouseDownEvent extends MouseEvent {}
export class MouseMoveEvent extends MouseEvent {
	deltaX: number;
	deltaY: number;
}
export class MouseUpEvent extends MouseEvent {}
export class MouseOverEvent extends MouseEvent {}

export class GraphicsItem {
	protected scene!: NodeScene;
	protected visible: boolean = true;

	protected x: number = 0;
	protected y: number = 0;
	protected width: number;
	protected height: number;

	public constructor() {
		//this.scene = scene;
		//scene.addItem(this);
		this.width = 1;
		this.height = 1;
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

	// UTILITIES
	// https://stackoverflow.com/questions/1255512/how-to-draw-a-rounded-rectangle-on-html-canvas
	roundRect(ctx: CanvasRenderingContext2D, x, y, w, h, r) {
		if (w < 2 * r) r = w / 2;
		if (h < 2 * r) r = h / 2;
		ctx.beginPath();
		ctx.moveTo(x + r, y);
		ctx.arcTo(x + w, y, x + w, y + h, r);
		ctx.arcTo(x + w, y + h, x, y + h, r);
		ctx.arcTo(x, y + h, x, y, r);
		ctx.arcTo(x, y, x + w, y, r);
		ctx.closePath();
		//ctx.stroke();
	}

	// to be overriden
	public draw(ctx: CanvasRenderingContext2D, renderData: any = null) {}

	// MOUSE EVENTS

	// STANDARD MOUSE EVENTS
	public mouseDown(evt: MouseDownEvent) {}
	public mouseMove(evt: MouseDownEvent) {}
	public mouseUp(evt: MouseDownEvent) {}

	// called every frame the mouse is over this object
	public mouseOver(evt: MouseOverEvent) {}
}
