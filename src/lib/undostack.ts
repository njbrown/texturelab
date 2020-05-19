export class Action {
	redo() {}

	undo() {}
}

//  https://gist.github.com/dsamarin/3050311
// https://github.com/agrinko/js-undo-manager
export class UndoStack {
	static current: UndoStack;

	stack: Action[];
	pointer: number;

	constructor() {
		this.pointer = -1;
		this.stack = [];
	}

	push(action: Action) {
		this.pointer += 1;
		this.stack.splice(this.pointer);

		this.stack.push(action);
		console.log(action);
	}

	undo() {
		if (this.pointer < 0) return;

		let action = this.stack[this.pointer];
		action.undo();

		this.pointer -= 1;
	}

	redo() {
		if (this.pointer >= this.stack.length - 1) return;

		this.pointer += 1;

		let action = this.stack[this.pointer];
		action.redo();
	}
}
