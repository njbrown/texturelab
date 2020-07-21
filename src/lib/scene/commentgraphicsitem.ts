import { SocketGraphicsItem } from "./socketgraphicsitem";
import {
	GraphicsItem,
	MouseDownEvent,
	MouseMoveEvent,
	MouseUpEvent
} from "./graphicsitem";
import { SceneView, Vector2 } from "./view";
import { Color } from "../designer/color";
import {
	IPropertyHolder,
	Property,
	StringProperty
} from "../designer/properties";
import { MoveItemsAction } from "../actions/moveItemsaction";
import { UndoStack } from "../undostack";

// https://stackoverflow.com/questions/5026961/html5-canvas-ctx-filltext-wont-do-line-breaks
export class CommentGraphicsItem extends GraphicsItem
	implements IPropertyHolder {
	text: string;
	textProp: StringProperty;
	view: SceneView;
	color: Color;

	padding: number;
	fontHeight: number;

	hit: boolean;
	dragged: boolean;
	dragStartPos: Vector2;

	constructor(view: SceneView) {
		super();
		this.text = "";
		this.view = view;
		this.color = new Color(0.9, 0.9, 0.9);

		this.hit = false;
		this.dragged = false;

		this.padding = 5;
		this.fontHeight = 20;

		this.textProp = new StringProperty("comment", "Comment", "Comment.", true);
		this.properties.push(this.textProp);

		this.setText("comment");
	}

	properties: Property[] = [];
	setProperty(name: string, value: any) {
		const prop = this.properties.find(x => {
			return x.name == name;
		});

		// if (prop) {
		// 	prop.setValue(value);
		// }

		if (name == "comment") {
			this.setText(value);
		}
	}

	setText(text: string) {
		this.text = text;
		this.textProp.setValue(text);
		const fontHeight = this.fontHeight;

		const ctx = this.view.context;

		ctx.lineWidth = 1;
		ctx.font = fontHeight + "px 'Open Sans'";
		const size = ctx.measureText(this.text);

		let maxWidth = 0;
		const lines = this.text.split("\n");
		// console.log(lines);
		// console.log(ctx);
		// console.log(ctx.font);
		for (let i = 0; i < lines.length; ++i) {
			const size = ctx.measureText(lines[i]);
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
		const col =
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
		const fontHeight = this.fontHeight;
		ctx.font = fontHeight + "px 'Open Sans'";
		ctx.fillStyle = "rgb(240, 240, 240)";

		// recalc rect
		let maxWidth = 0;
		//console.log(this.text);
		const lines = this.text.split("\n");
		for (var i = 0; i < lines.length; ++i) {
			const size = ctx.measureText(lines[i]);
			maxWidth = Math.max(maxWidth, size.width);
		}

		this.width = maxWidth + this.padding * 2;
		this.height = lines.length * fontHeight + this.padding * 2;

		// --------------------------------------------------------

		const width = this.width;
		const height = this.height;

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
		const textX = this.x + this.padding;
		let textY = this.y + fontHeight;

		const lineHeight = fontHeight;
		//var lines = this.text.split("\n");
		ctx.font = fontHeight + "px 'Open Sans'";
		ctx.textAlign = "left";
		ctx.lineWidth = 1;

		//console.log(ctx.font);
		for (let i = 0; i < lines.length; ++i) {
			ctx.fillText(lines[i], textX, textY);
			textY += lineHeight;
			const size = ctx.measureText(lines[i]);
			//console.log("RENDER WITH: " + size.width);
		}
	}

	// MOUSE EVENTS
	public mouseDown(evt: MouseDownEvent) {
		this.hit = true;
		this.dragged = false;
		this.dragStartPos = new Vector2(this.x, this.y);
		//console.log(this.text);
	}

	public mouseMove(evt: MouseMoveEvent) {
		if (this.hit) {
			// movement
			this.move(evt.deltaX, evt.deltaY);
			this.dragged = true;
		}
	}

	public mouseUp(evt: MouseUpEvent) {
		this.hit = false;

		// add undo/redo
		const newPos = new Vector2(this.x, this.y);

		if (this.dragged) {
			const action = new MoveItemsAction(
				[this],
				[this.dragStartPos.clone()],
				[newPos]
			);

			UndoStack.current.push(action);
		}

		this.dragged = false;
	}
}
