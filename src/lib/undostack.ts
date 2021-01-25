export class Action {
	redo() {}

	undo() {}
}

//  https://gist.github.com/dsamarin/3050311
// https://github.com/agrinko/js-undo-manager
// https://doc.qt.io/qt-5/qundostack.html#:~:text=An%20undo%20stack%20maintains%20a,createUndoAction()%20and%20createRedoAction().
export class UndoStack {
	static current: UndoStack;

	stack: Action[];
	pointer: number;

	// this tracks the last clean state
	savedState: number = -1;
	cleanStatusChanged: (isClean: boolean) => void = null;

	constructor() {
		this.pointer = -1;
		this.stack = [];
	}

	push(action: Action) {
		let isClean = this.isClean();

		this.pointer += 1;
		this.stack.splice(this.pointer);

		this.stack.push(action);
		console.log(action);

		if (isClean != this.isClean() && this.cleanStatusChanged) {
			this.cleanStatusChanged(this.isClean());
		}
	}

	undo() {
		if (this.pointer < 0) return;

		let isClean = this.isClean();

		const action = this.stack[this.pointer];
		action.undo();

		this.pointer -= 1;

		if (isClean != this.isClean() && this.cleanStatusChanged) {
			this.cleanStatusChanged(this.isClean());
		}
	}

	redo() {
		if (this.pointer >= this.stack.length - 1) return;

		let isClean = this.isClean();

		this.pointer += 1;

		const action = this.stack[this.pointer];
		action.redo();

		if (isClean != this.isClean() && this.cleanStatusChanged) {
			this.cleanStatusChanged(this.isClean());
		}
	}

	clear() {
		let isClean = this.isClean();

		this.pointer = -1;
		this.savedState = -1;
		this.stack = [];

		if (!isClean) {
			this.cleanStatusChanged(true);
		}
	}

	setClean() {
		this.savedState = this.pointer;
	}

	isClean() {
		return this.savedState == this.pointer;
	}
}
