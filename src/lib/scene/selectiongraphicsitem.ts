import { SocketGraphicsItem } from "./socketgraphicsitem";
import {
	GraphicsItem,
	MouseDownEvent,
	MouseMoveEvent,
	MouseUpEvent,
} from "./graphicsitem";
import { SceneView, Vector2, Rect } from "./view";
import { Color } from "../designer/color";
import {
	IPropertyHolder,
	Property,
	StringProperty,
} from "../designer/properties";
import { NodeScene } from "../scene";
import { MoveItemsAction } from "../actions/moveItemsaction";
import { UndoStack } from "../undostack";
import { FrameGraphicsItem } from "./framegraphicsitem";

// https://stackoverflow.com/questions/5026961/html5-canvas-ctx-filltext-wont-do-line-breaks
export class SelectionGraphicsItem extends GraphicsItem {
	view: SceneView;
	color: Color;

	padding: number;
	fontHeight: number;

	tooSmallSize: number;

	hit: boolean;
	dragged: boolean;
	items: GraphicsItem[];
	draggableItems: GraphicsItem[]; // list with items and frame's items for dragging
	itemsDragStartPos: Vector2[];

	constructor(scene: NodeScene, view: SceneView) {
		super();
		this.scene = scene;
		this.view = view;
		this.color = new Color(0.9, 0.9, 0.9);
		this.items = new Array();
		this.itemsDragStartPos = new Array();

		this.hit = false;
		this.dragged = false;

		this.padding = 5;
		this.fontHeight = 20;

		this.tooSmallSize = 5;
	}

	public isPointInside(px: number, py: number): boolean {
		//todo: loop through child items rect to see if a hit is made
		for (let item of this.items) {
			if (
				px >= item.left &&
				px <= item.left + item.getWidth() &&
				py >= item.top &&
				py <= item.top + item.getHeight()
			)
				return true;
		}

		return false;
	}

	draw(ctx: CanvasRenderingContext2D, renderData: any = null) {
		// should only display if hit or has items
		if (this.hit == false && this.items.length == 0) return;

		let width = this.width;
		let height = this.height;

		if (this.items.length == 0) {
			if (!this.isTooSmall()) {
				// stroke bounding rect
				ctx.beginPath();
				ctx.lineWidth = 3;
				ctx.strokeStyle = "rgb(250, 250, 250)";
				//this.roundRect(ctx, this.x, this.y, width, height, 1);
				ctx.rect(this.x, this.y, width, height);

				ctx.setLineDash([5, 3]);
				ctx.stroke();
				ctx.setLineDash([]);

				// if hit, then mouse is being dragged, these items are temporary
				let items = this.getHitItems();
				this.drawSelectedItems(items, ctx);
			}
		} else {
			//this.drawSelectedItems(this.items, ctx);
		}
	}

	drawSelectedItems(items: GraphicsItem[], ctx: CanvasRenderingContext2D) {
		for (let item of items) {
			ctx.beginPath();
			ctx.lineWidth = 2;
			ctx.strokeStyle = "rgba(250, 250, 250)";
			//this.roundRect(ctx, this.x, this.y, width, height, 1);
			// ctx.rect(item.left, item.top, item.getWidth(), item.getHeight());
			var rect = item.getRect();
			rect.expand(15);
			ctx.rect(rect.left, rect.top, rect.width, rect.height);

			ctx.stroke();

			ctx.fillStyle = "rgba(255, 255, 255, 0.1)";
			ctx.rect(rect.left, rect.top, rect.width, rect.height);
			ctx.fill();
		}
		ctx.setLineDash([]);
	}

	/**
	 * If user drags left of up, the width or height becomes negative
	 * This returns a rect with positive dimensions
	 */
	getPositiveRect(): Rect {
		let rect = new Rect();
		rect.x = this.x;
		rect.y = this.y;

		rect.width = Math.abs(this.width);
		rect.height = Math.abs(this.height);

		if (this.width < 0) rect.x += this.width;
		if (this.height < 0) rect.y += this.height;

		return rect;
	}

	// for manually setting the hit items
	setHitItems(items: GraphicsItem[]) {
		this.items = items;
		this.draggableItems = this.getDraggableHitItems(this.items);
	}

	getHitItems(): GraphicsItem[] {
		let items: GraphicsItem[] = [];
		let rect = this.getPositiveRect();

		for (let node of this.scene.nodes) {
			if (node.intersectsRect(rect)) {
				items.push(node);
			}
		}

		for (let item of this.scene.comments) {
			if (item.intersectsRect(rect)) {
				items.push(item);
			}
		}

		// Frames are treated differently
		// check if any of the sides were hit
		// by the selection box for a valid selection
		for (let item of this.scene.frames) {
			let frame = <FrameGraphicsItem>item;
			let regions = frame.getFrameRegions();
			for (let region of regions) {
				if (region.rect.intersects(rect)) {
					items.push(item);
					break;
				}
			}
		}

		for (let item of this.scene.navigations) {
			if (item.intersectsRect(rect)) {
				items.push(item);
			}
		}

		return items;
	}

	// todo: ultra slow!
	// does frame * scenenodes check per frame
	getDraggableHitItems(hitItems: GraphicsItem[]): GraphicsItem[] {
		let items: Set<GraphicsItem> = new Set<GraphicsItem>();

		hitItems.forEach((i: GraphicsItem) => {
			items.add(i);

			if (i instanceof FrameGraphicsItem) {
				let captured = (<FrameGraphicsItem>i).getHoveredNodes();
				captured.forEach((c) => items.add(c));
			}
		});

		let results: GraphicsItem[] = [];
		items.forEach((i: GraphicsItem) => results.push(i));

		return results;
	}

	// MOUSE EVENTS

	// This graphics item has two phases
	// the first phase is about selecting items
	// the second phase is about dragging
	// after the first phase, items will not be empty
	// if there is a mouse event and items is not empty
	// then its in drag mode
	public mouseDown(evt: MouseDownEvent) {
		this.hit = true;
		this.dragged = false;

		if (this.items.length > 0) {
			this.captureDragStarts();
		} else {
			this.x = evt.globalX;
			this.y = evt.globalY;
			this.scene.setSelectedItems([]);
		}
	}

	public mouseMove(evt: MouseMoveEvent) {
		if (this.items.length > 0) {
			for (let item of this.draggableItems) {
				item.move(evt.deltaX, evt.deltaY);
			}
			this.dragged = true;
		} else if (this.hit) {
			// movement
			this.width += evt.deltaX;
			this.height += evt.deltaY;
		}
	}

	public mouseUp(evt: MouseUpEvent) {
		this.hit = false;
		if (this.items.length == 0 && !this.isTooSmall()) {
			this.items = this.getHitItems();
			this.scene.setSelectedItems(this.items);
			this.draggableItems = this.getDraggableHitItems(this.items);
		} else {
			//todo: check for movement
			if (this.dragged) this.createUndoAction();
		}

		this.dragged = false;
	}

	// UNDO-REDO
	captureDragStarts() {
		this.itemsDragStartPos = [];
		for (let item of this.draggableItems) {
			let pos = new Vector2(item.left, item.top);
			this.itemsDragStartPos.push(pos);
		}
	}

	isTooSmall(): boolean {
		return (
			Math.abs(this.width) < this.tooSmallSize ||
			Math.abs(this.height) < this.tooSmallSize
		);
	}

	createUndoAction() {
		let newPosList = [];
		for (let item of this.draggableItems) {
			let pos = new Vector2(item.left, item.top);
			newPosList.push(pos);
		}

		let action = new MoveItemsAction(
			this.draggableItems,
			this.itemsDragStartPos,
			newPosList
		);

		console.log(action);

		UndoStack.current.push(action);
	}
}
