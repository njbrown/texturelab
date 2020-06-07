import { NodeScene } from "../scene";
import { Rect, Vector2 } from "./view";

export class MouseEvent {
	// scene space
	globalX: number;
	globalY: number;

	localX: number;
	localY: number;

	mouseButton: number;

	// modifiers
	shiftKey: boolean = false;
	altKey: boolean = false;
	ctrlKey: boolean = false;

	// default is accepted
	private accepted: boolean = true;
	public accept() {
		this.accepted = true;
	}

	public reject() {
		this.accepted = false;
	}

	public get isAccepted() {
		return this.accepted;
	}

	public get isRejected() {
		return !this.accepted;
	}
}

export class MouseDownEvent extends MouseEvent {}
export class MouseMoveEvent extends MouseEvent {
	deltaX: number;
	deltaY: number;
}
export class MouseUpEvent extends MouseEvent {}
export class MouseOverEvent extends MouseEvent {}

export class HoverEvent extends MouseEvent {}

export class GraphicsItem {
	scene!: NodeScene;
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

	// sets top-left
	public setPos(x: number, y: number) {
		this.x = x;
		this.y = y;
	}

	public getPos() {
		return new Vector2(this.x, this.y);
	}

	public setSize(w: number, h: number) {
		this.width = w;
		this.height = h;
	}

	public setScene(scene: NodeScene) {
		this.scene = scene;
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

	public get left() {
		return this.x;
	}

	public set left(value) {
		this.x = value;
	}

	public get top() {
		return this.y;
	}

	public set top(value) {
		this.y = value;
	}

	public get right() {
		return this.x + this.width;
	}

	public get bottom() {
		return this.y + this.height;
	}

	public intersectsRect(other: Rect) {
		if (this.left > other.right) return false;
		if (this.right < other.left) return false;
		if (this.bottom < other.top) return false;
		if (this.top > other.bottom) return false;

		return true;
	}

	public intersects(other: GraphicsItem) {
		if (this.left > other.right) return false;
		if (this.right < other.left) return false;
		if (this.bottom < other.top) return false;
		if (this.top > other.bottom) return false;

		return true;
	}

	public getRect(): Rect {
		let rect = new Rect();
		rect.x = this.x;
		rect.y = this.y;
		rect.width = this.width;
		rect.height = this.height;

		return rect;
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

	public getWidth(): number {
		return this.width;
	}

	public getHeight(): number {
		return this.height;
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
	public mouseMove(evt: MouseMoveEvent) {}
	public mouseUp(evt: MouseUpEvent) {}

	// called every frame the mouse is over this object
	public mouseOver(evt: MouseOverEvent) {}
}
