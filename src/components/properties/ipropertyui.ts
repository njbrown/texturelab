export interface IProperyUi {
	refresh();
}

export class PropertyChangeComplete {
	public propName: string;
	public oldValue: any;
	public newValue: any;
}
