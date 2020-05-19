import { Action } from "../undostack";
import { GraphicsItem } from "../scene/graphicsitem";
import { Vector2, Rect } from "../scene/view";
import { FrameGraphicsItem } from "../scene/framegraphicsitem";
import { Editor } from "../editortest";
import { IPropertyHolder } from "../designer/properties";
import { IProperyUi } from "@/components/properties/ipropertyui";
import App from "@/App.vue";

export class PropertyChangeAction extends Action {
	ui: IProperyUi;
	propHolder: IPropertyHolder;
	propName: string;
	oldValue: any;
	newValue: any;

	constructor(
		ui: IProperyUi,
		propName: string,
		propHolder: IPropertyHolder,
		oldValue: any,
		newValue: any
	) {
		super();

		this.ui = ui;
		this.propName = propName;
		this.propHolder = propHolder;
		this.oldValue = oldValue;
		this.newValue = newValue;
	}

	undo() {
		this.propHolder.setProperty(this.propName, this.oldValue);
		//App.instance.$refs.properties.$forceUpdate();
		//this.ui.refresh();
	}

	redo() {
		this.propHolder.setProperty(this.propName, this.newValue);
		//this.ui.refresh();
	}
}
