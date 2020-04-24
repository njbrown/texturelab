import { Action } from "../undostack";
import { GraphicsItem } from "../scene/graphicsitem";
import { Vector2, Rect } from "../scene/view";
import { FrameGraphicsItem } from "../scene/framegraphicsitem";
import { Editor } from "../editortest";
import { IPropertyHolder } from "../designer/properties";

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
export class PropertyChangeAction extends Action {
	editor: Editor;
	propHolder: IPropertyHolder;
	propName: string;
	oldValue: object;
	newValue: object;

	constructor(
		editor: Editor,
		propName: string,
		propHolder: IPropertyHolder,
		oldValue: object,
		newValue: object
	) {
		super();

		this.editor = editor;
		this.propName = propName;
		this.propHolder = propHolder;
		this.oldValue = oldValue;
		this.newValue = newValue;
	}

	undo() {
		this.propHolder.setProperty(this.propName, this.oldValue);
	}

	redo() {
		this.propHolder.setProperty(this.propName, this.newValue);
	}
}
