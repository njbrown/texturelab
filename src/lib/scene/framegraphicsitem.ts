import { SocketGraphicsItem } from "./socketgraphicsitem";
import { GraphicsItem } from "./graphicsitem";
import { SceneView } from "./view";

export class FrameGraphicsItem extends GraphicsItem {
	title: string;
	description: string;
	showTitle: boolean;
	view: SceneView;

	public constructor(view: SceneView) {
		super();
		this.title = "Frame";
		this.description = "";
		this.showTitle = true;
		this.view = view;
	}

	setSize(w: number, h: number) {
		this.width = w;
		this.height = h;
	}

	draw(ctx: CanvasRenderingContext2D, renderData: any = null) {
		// frame
		ctx.beginPath();
		ctx.lineWidth = 4;
		ctx.strokeStyle = "rgb(255, 0, 0)";
		this.roundRect(ctx, this.x, this.y, this.width, this.height, 1);
		ctx.stroke();

		// handle
		let handleSize = 30;
		ctx.beginPath();
		ctx.lineWidth = 4;
		ctx.strokeStyle = "rgb(255, 0, 0)";
		ctx.fillStyle = "rgb(255, 0, 0)";
		this.roundRect(ctx, this.x, this.y, this.width, 30, 1);
		ctx.fill();

		// title
		if (true == true) {
			ctx.beginPath();

			let fontSize = 12 * this.view.zoomFactor;

			ctx.save();
			//ctx.scale(1.0 / this.view.zoomFactor, 1.0 / this.view.zoomFactor);
			ctx.setTransform(
				1,
				0,
				0,
				1,
				this.view.offset.x,
				this.view.offset.y
			);

			//ctx.font = fontSize + "px 'Open Sans'";
			ctx.font = "18px 'Open Sans'";
			ctx.fillStyle = "rgb(200, 255, 200)";
			//let size = ctx.measureText(this.textureChannel.toUpperCase());
			let textX = this.x;
			let textY = this.y;
			ctx.fillText("Hello World", textX, textY);

			ctx.restore();
		}
	}
}
