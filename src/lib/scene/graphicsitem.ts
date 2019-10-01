import { NodeScene } from "../scene";

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

	// to be overriden
	public draw(ctx: CanvasRenderingContext2D, renderData: any = null) {}
}
