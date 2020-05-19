import { Action } from "../undostack";
import { GraphicsItem } from "../scene/graphicsitem";
import { Vector2, Rect } from "../scene/view";
import { FrameGraphicsItem } from "../scene/framegraphicsitem";

export class ResizeFrameAction extends Action {
	frame: FrameGraphicsItem;
	oldRect: Rect;
	newRect: Rect;

	constructor(frame: FrameGraphicsItem, oldRect: Rect, newRect: Rect) {
		super();

		this.frame = frame;
		this.oldRect = oldRect;
		this.newRect = newRect;
	}

	undo() {
		this.frame.setFrameRect(this.oldRect);
	}

	redo() {
		this.frame.setFrameRect(this.newRect);
	}
}
