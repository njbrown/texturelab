export class PropGroupCache {
	// private static _instance;
	// public static get instance() {
	// 	if (PropCache._instance == null) PropCache._instance = new PropCache();

	// 	return PropCache._instance;
	// }

	static states: Map<string, Map<string, boolean>> = new Map<
		string,
		Map<string, boolean>
	>();

	static getCollapseState(
		nodeType: string,
		groupName: string,
		defaultValue: boolean
	): boolean {
		const states = PropGroupCache.states;

		if (!states.has(nodeType)) return defaultValue;

		const nodeStates = states.get(nodeType);

		if (!nodeStates.has(groupName)) return defaultValue;

		return nodeStates.get(groupName);
	}

	static setCollapseState(
		nodeType: string,
		groupName: string,
		val: boolean
	): void {
		const states = PropGroupCache.states;

		let nodeStates: Map<string, boolean> = null;
		if (states.has(nodeType)) nodeStates = states.get(nodeType);
		else nodeStates = new Map<string, boolean>();

		nodeStates.set(groupName, val);

		states.set(nodeType, nodeStates);
	}
}
