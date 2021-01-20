import { Action } from "../undostack";
import { DesignerNode } from "../designer/designernode";

export class SetNodeRandomSeedAction extends Action {
	ui: () => void;
	node: DesignerNode;
	oldSeed: number;
	newSeed: number;

	constructor(ui: () => void, node: DesignerNode, oldSeed: number, newSeed: number) {
		super();

		this.ui = ui;
		this.node = node;
		this.oldSeed = oldSeed;
		this.newSeed = newSeed;
	}

	undo() {
		this.node.setRandomSeed(this.oldSeed);
		if (this.ui) this.ui();
	}

	redo() {
		this.node.setRandomSeed(this.newSeed);
		if (this.ui) this.ui();
	}
}
