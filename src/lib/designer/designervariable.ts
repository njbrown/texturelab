import { DesignerNode } from "./designernode";
import { Property } from "./properties";

export enum DesignerVariableType {
	None = 0,
	Float = 1,
	Int = 2,
	Bool = 3,
	Enum = 4,
	Color = 5
	//Gradient
}

export class DesignerNodePropertyMap {
	public node: DesignerNode;
	public propertyName: string;
}

export class DesignerVariable {
	id: string;

	type: DesignerVariableType;
	// used to keep the value in bounds
	property: Property;

	nodes: DesignerNodePropertyMap[] = [];
}
