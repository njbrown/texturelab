import { Action } from "../undostack";
import { GraphicsItem } from "../scene/graphicsitem";
import { Vector2 } from "../scene/view";
import { ConnectionGraphicsItem } from "../scene/connectiongraphicsitem";
import { NodeScene } from "../scene";

export class RemoveConnectionAction extends Action {
	scene: NodeScene;
	con: ConnectionGraphicsItem;

	constructor(scene: NodeScene, con: ConnectionGraphicsItem) {
		super();

		this.scene = scene;
		this.con = con;
	}

	undo() {
		this.scene.addConnection(this.con);
	}

	redo() {
		this.scene.removeConnection(this.con);
	}
}
