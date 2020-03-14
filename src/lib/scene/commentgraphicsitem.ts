import { SocketGraphicsItem } from "./socketgraphicsitem";
import { GraphicsItem } from "./graphicsitem";
import { SceneView } from "./view";
import { Color } from "../designer/color";

// https://stackoverflow.com/questions/5026961/html5-canvas-ctx-filltext-wont-do-line-breaks
export class CommentGraphicsItem extends GraphicsItem {
	text: string;
	view: SceneView;
	color: Color;

	padding: number;
	fontHeight: number;

	constructor(view: SceneView) {
		super();
		this.text = "";
		this.view = view;
		this.color = new Color(0.9, 0.9, 0.9);

		this.padding = 5;
		this.fontHeight = 20;
	}

	setText(text: string) {
		this.text = text;
		let fontHeight = this.fontHeight;

		let ctx = this.view.context;

		ctx.lineWidth = 1;
		ctx.font = fontHeight + "px 'Open Sans'";
		let size = ctx.measureText(this.text);

		var maxWidth = 0;
		var lines = this.text.split("\n");
		// console.log(lines);
		// console.log(ctx);
		// console.log(ctx.font);
		for (var i = 0; i < lines.length; ++i) {
			let size = ctx.measureText(lines[i]);
			//console.log("INITIAL WITH: " + size.width);
			maxWidth = Math.max(maxWidth, size.width);
		}

		// somewhat inaccurate here for some reason
		// maybe some bug in html5 canvas
		// recalculate in draw function
		this.width = maxWidth + this.padding * 2;
		this.height = lines.length * fontHeight + this.padding * 2;
	}

	private buildColor(color: Color, alpha: number) {
		var col =
			"rgba(" +
			color.r * 255 +
			"," +
			color.g * 255 +
			"," +
			color.b * 255 +
			"," +
			alpha +
			")";

		return col;
	}

	draw(ctx: CanvasRenderingContext2D, renderData: any = null) {
		let fontHeight = this.fontHeight;
		ctx.font = fontHeight + "px 'Open Sans'";
		ctx.fillStyle = "rgb(240, 240, 240)";

		// recalc rect
		let maxWidth = 0;
		let lines = this.text.split("\n");
		for (var i = 0; i < lines.length; ++i) {
			let size = ctx.measureText(lines[i]);
			maxWidth = Math.max(maxWidth, size.width);
		}

		this.width = maxWidth + this.padding * 2;
		this.height = lines.length * fontHeight + this.padding * 2;

		// --------------------------------------------------------

		let width = this.width;
		let height = this.height;

		// stroke bounding rect
		ctx.beginPath();
		ctx.lineWidth = 1;
		ctx.strokeStyle = this.buildColor(this.color, 0.5);
		this.roundRect(ctx, this.x, this.y, width, height, 1);
		ctx.stroke();

		// inner area
		ctx.beginPath();
		ctx.lineWidth = 1;
		ctx.fillStyle = this.buildColor(this.color, 0.1);
		this.roundRect(ctx, this.x, this.y, width, height, 1);
		ctx.fill();

		// multiline text
		ctx.fillStyle = "rgb(240, 240, 240)";
		let textX = this.x + this.padding;
		let textY = this.y + fontHeight;

		let lineHeight = fontHeight;
		//var lines = this.text.split("\n");
		ctx.font = fontHeight + "px 'Open Sans'";
		ctx.textAlign = "left";
		ctx.lineWidth = 1;

		console.log(ctx.font);
		for (var i = 0; i < lines.length; ++i) {
			ctx.fillText(lines[i], textX, textY);
			textY += lineHeight;
			let size = ctx.measureText(lines[i]);
			//console.log("RENDER WITH: " + size.width);
		}
	}
}
