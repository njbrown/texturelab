import { SocketGraphicsItem } from "./socketgraphicsitem";
import { GraphicsItem } from "./graphicsitem";

export class NavigationGraphicsItem extends GraphicsItem {
	id!: string;
	public socketA!: SocketGraphicsItem;
	public socketB!: SocketGraphicsItem;

	draw(ctx: CanvasRenderingContext2D, renderData: any = null) {
		ctx.beginPath();
		ctx.strokeStyle = "rgb(200, 200, 200)";
		ctx.lineWidth = 4;
		ctx.moveTo(this.socketA.centerX(), this.socketA.centerY());
		ctx.bezierCurveTo(
			this.socketA.centerX() + 60,
			this.socketA.centerY(), // control point 1
			this.socketB.centerX() - 60,
			this.socketB.centerY(),
			this.socketB.centerX(),
			this.socketB.centerY()
		);
		ctx.stroke();
	}
}
