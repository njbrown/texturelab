import { DesignerNode } from "./designernode";

export class DesignerNodeFactory {
	public name: string;
	public displayName: string;
	public create: () => DesignerNode;
}

// holds list of node factories
export class DesignerLibrary {
	versionName: string;
	nodes = [];

	// https://www.snip2code.com/Snippet/685188/Create-instance-of-generic-type-on-TypeS
	public addNode<T extends DesignerNode>(
		name: string,
		displayName: string,
		type: { new (): T }
	) {
		const factory = new DesignerNodeFactory();
		factory.name = name;
		factory.displayName = displayName;
		factory.create = (): DesignerNode => {
			return new type();
		};

		//this.nodes.push(factory);
		this.nodes[name] = factory;
	}

	getVersionName() {
		return this.versionName;
	}

	public create(name: string): DesignerNode {
		//if (this.nodes.indexOf(name) == -1)
		//    return null;

		const node = this.nodes[name].create();
		node.typeName = name;

		return node;
	}
}
