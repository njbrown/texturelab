import { Action } from "../undostack";
import { GraphicsItem } from "../scene/graphicsitem";
import { Vector2, Rect } from "../scene/view";
import { FrameGraphicsItem } from "../scene/framegraphicsitem";
import { Editor } from "../editortest";

/*
FROM

node1 --> channel1

ndoe2 --> channel2

OR

node1 --> channel1

      --> channel2


TO

node1 \	  channel1
       \
node2   \-> channel2

OR

node1 \	  channel1
       \
        \-> channel2

NOTE:
node2 might be null
if channel2 is null then we just remove the channel
*/
export class ChangeTextureChannelAction extends Action {
	editor: Editor;
	node1: string;
	node2: string;
	channel1: string;
	channel2: string;

	constructor(
		editor: Editor,
		node1: string, // active node
		channel1: string, // current channel
		node2: string, // new channel
		channel2: string // node being displaced from new channel
	) {
		super();

		this.editor = editor;
		this.channel1 = channel1;
		this.channel2 = channel2;
		this.node1 = node1;
		this.node2 = node2;
	}

	undo() {
		if (this.channel1) {
			this.editor.assignNodeToTextureChannel(this.node1, this.channel1);
		} else {
			// node didnt have a channel to begin with
			this.editor.clearTextureChannel(this.node1);
		}

		if (this.channel2 != null && this.node2 != null) {
			// reset displaced node
			this.editor.assignNodeToTextureChannel(this.node2, this.channel2);
		}
	}

	redo() {
		if (this.channel2 == null) {
			// clear channel
			this.editor.clearTextureChannel(this.node1);
		} else {
			this.editor.assignNodeToTextureChannel(this.node1, this.channel2);
		}
	}
}
