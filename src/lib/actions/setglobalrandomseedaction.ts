import { Action } from "../undostack";
import { GraphicsItem } from "../scene/graphicsitem";
import { Vector2, Rect } from "../scene/view";
import { FrameGraphicsItem } from "../scene/framegraphicsitem";
import { Editor } from "../editor";
import { IApp } from "@/iapp";

export class SetGlobalRandomSeedAction extends Action {
	editor: Editor;
	oldSeed: number;
	newSeed: number;
	app:IApp;

	constructor(app:IApp, editor: Editor, oldSeed: number, newSeed: number) {
		super();

		this.app = app;
		this.editor = editor;
		this.oldSeed = oldSeed;
		this.newSeed = newSeed;
	}

	undo() {
		this.app.randomSeed = this.oldSeed;
		this.editor.designer.setRandomSeed(this.oldSeed);
	}

	redo() {
		this.app.randomSeed = this.newSeed;
		this.editor.designer.setRandomSeed(this.newSeed);
	}
}
