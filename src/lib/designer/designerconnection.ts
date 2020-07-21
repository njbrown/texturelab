import { DesignerNode } from "./designernode";
import { Guid } from "../utils";

export class DesignerNodeConn {
	public id: string = Guid.newGuid();

	leftNode: DesignerNode;
	leftNodeOutput = ""; // if null, use first output

	rightNode: DesignerNode;
	rightNodeInput: string;
}
