import { Action } from "../undostack";
import { GraphicsItem } from "../scene/graphicsitem";
import { Vector2 } from "../scene/view";
import { ConnectionGraphicsItem } from "../scene/connectiongraphicsitem";
import { NodeScene } from "../scene";
import { FrameGraphicsItem } from "../scene/framegraphicsitem";
import { CommentGraphicsItem } from "../scene/commentgraphicsitem";
import { NodeGraphicsItem } from "../scene/nodegraphicsitem";
import { NavigationGraphicsItem } from "../scene/navigationgraphicsitem";
import { DesignerNode } from "../designer/designernode";
import { Designer } from "../designer";
import { Editor } from "../editortest";

// used when items are added to the scene
// also used for paste events
export class RemoveItemsAction extends Action {
	editor: Editor;
	scene: NodeScene;
	designer: Designer;
	frames: FrameGraphicsItem[];
	comments: CommentGraphicsItem[];
	navs: NavigationGraphicsItem[];
	cons: ConnectionGraphicsItem[];
	nodes: NodeGraphicsItem[];
	dnodes: DesignerNode[];
	textureChannels: Map<string, string>;

	constructor(
		editor: Editor,
		scene: NodeScene,
		designer: Designer,
		frames: FrameGraphicsItem[],
		comments: CommentGraphicsItem[],
		navs: NavigationGraphicsItem[],
		cons: ConnectionGraphicsItem[],
		nodes: NodeGraphicsItem[],
		dnodes: DesignerNode[]
	) {
		super();

		this.editor = editor;
		this.scene = scene;
		this.designer = designer;
		this.frames = frames;
		this.comments = comments;
		this.navs = navs;
		this.cons = cons;
		this.nodes = nodes;
		this.dnodes = dnodes;

		this.textureChannels = new Map<string, string>();

		for (let node of nodes) {
			if (node.textureChannel != null) {
				this.textureChannels.set(node.id, node.textureChannel);
			}
		}
	}

	undo() {
		for (let frame of this.frames) {
			this.scene.addFrame(frame);
		}

		for (let comment of this.comments) {
			this.scene.addComment(comment);
		}

		for (let nav of this.navs) {
			this.scene.addNavigation(nav);
		}

		for (let node of this.nodes) {
			this.scene.addNode(node);
		}

		for (let dnode of this.dnodes) {
			this.designer.addNode(dnode, false);
		}

		// assign texture channels
		for (let [nodeId, channel] of this.textureChannels) {
			this.editor.assignNodeToTextureChannel(nodeId, channel);
		}

		// relying on callbacks to add the connection
		// in designer
		for (let con of this.cons) {
			this.scene.addConnection(con);
		}
	}

	redo() {
		for (let frame of this.frames) {
			this.scene.removeFrame(frame);
		}

		for (let comment of this.comments) {
			this.scene.removeComment(comment);
		}

		for (let nav of this.navs) {
			this.scene.removeNavigation(nav);
		}

		// for (let dnode of this.dnodes) {
		// 	this.designer.addNode(dnode, false);
		// }

		// relying on callbacks to add the connection
		// in designer
		for (let con of this.cons) {
			this.scene.removeConnection(con);
		}

		// also relying callbacks to have the node deleted in designer
		// note: texture channel gets removed here if assigned
		//       since texture channels are not assigned upon a node being
		//		 added to a scene then there is no need to attempt to
		//		 reassign any
		for (let node of this.nodes) {
			this.scene.deleteNode(node);
		}
	}
}
