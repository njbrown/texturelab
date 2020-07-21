import { Color } from "./color";
import { Gradient } from "./gradient";

// for use in code after build
export enum PropertyType {
	Float = "float",
	Int = "int",
	Bool = "bool",
	Color = "color",
	Enum = "enum",
	String = "string",
	Gradient = "gradient"
}

export class Property {
	public name: string;
	public displayName: string;
	public type: string;

	// to be overriden
	public getValue(): any {
		return null;
	}

	public setValue(val: any) {}

	public clone(): Property {
		return null;
	}
}

export interface IPropertyHolder {
	properties: Property[];
	setProperty(name: string, value: any);
}

export class FloatProperty extends Property {
	value: number;
	minValue = 0;
	maxValue = 1;
	step = 1;
	public constructor(
		name: string,
		displayName: string,
		value: number,
		step = 1
	) {
		super();
		this.name = name;
		this.displayName = displayName;
		this.value = value;
		this.step = step;
		this.type = PropertyType.Float;
	}

	public getValue(): any {
		return this.value;
	}

	public setValue(val: any) {
		// todo: validate
		this.value = val;
	}

	public clone(): Property {
		const prop = new FloatProperty(
			this.name,
			this.displayName,
			this.value,
			this.step
		);
		prop.minValue = this.minValue;
		prop.maxValue = this.maxValue;

		return prop;
	}

	public copyValuesFrom(prop: FloatProperty) {
		this.minValue = prop.minValue;
		this.maxValue = prop.maxValue;
		this.value = prop.value;
		this.step = prop.step;
	}
}

export class IntProperty extends Property {
	value: number;
	minValue = 0;
	maxValue = 100;
	step = 1;
	public constructor(
		name: string,
		displayName: string,
		value: number,
		step = 1
	) {
		super();
		this.name = name;
		this.displayName = displayName;
		this.value = value;
		this.step = step;
		this.type = PropertyType.Int;
	}

	public getValue(): any {
		return this.value;
	}

	public setValue(val: any) {
		// todo: validate
		this.value = val;
	}

	public clone(): Property {
		const prop = new IntProperty(
			this.name,
			this.displayName,
			this.value,
			this.step
		);
		prop.minValue = this.minValue;
		prop.maxValue = this.maxValue;

		return prop;
	}

	public copyValuesFrom(prop: IntProperty) {
		this.minValue = prop.minValue;
		this.maxValue = prop.maxValue;
		this.value = prop.value;
		this.step = prop.step;
	}
}

export class BoolProperty extends Property {
	value: boolean;
	public constructor(name: string, displayName: string, value: boolean) {
		super();
		this.name = name;
		this.displayName = displayName;
		this.value = value;
		this.type = PropertyType.Bool;
	}

	public getValue(): any {
		return this.value;
	}

	public setValue(val: any) {
		// todo: validate
		this.value = val;
	}

	public clone(): Property {
		const prop = new BoolProperty(this.name, this.displayName, this.value);

		return prop;
	}

	public copyValuesFrom(prop: BoolProperty) {
		this.value = prop.value;
	}
}

export class EnumProperty extends Property {
	values: string[];
	index = 0;
	public constructor(name: string, displayName: string, values: string[]) {
		super();
		this.name = name;
		this.displayName = displayName;
		this.values = values;
		this.type = PropertyType.Enum;
	}

	public getValues(): string[] {
		return this.values;
	}

	public getValue(): any {
		return this.index;
	}

	public setValue(val: any) {
		// todo: validate
		this.index = val;
	}

	public clone(): Property {
		const prop = new EnumProperty(
			this.name,
			this.displayName,
			this.values.slice(0)
		);
		prop.index = this.index;

		return prop;
	}

	public copyValuesFrom(prop: EnumProperty) {
		this.values = prop.values;
		this.index = prop.index;
	}
}

export class ColorProperty extends Property {
	value: Color;
	public constructor(name: string, displayName: string, value: Color) {
		super();
		this.name = name;
		this.displayName = displayName;
		this.value = value;
		this.type = PropertyType.Color;
	}

	public getValue(): any {
		return this.value;
	}

	public setValue(val: any) {
		// todo: validate
		console.log("got color: " + val);
		if (val instanceof Color) this.value = val;
		else if (typeof val == "string") this.value = Color.parse(val);
		else if (typeof val == "object") {
			console.log("setting value", val);
			const value = new Color();
			value.r = val.r || 0;
			value.g = val.g || 0;
			value.b = val.b || 0;
			value.a = val.a || 1.0;

			this.value = value;
		}
	}

	public clone(): Property {
		const prop = new ColorProperty(this.name, this.displayName, this.value);

		return prop;
	}

	public copyValuesFrom(prop: ColorProperty) {
		this.setValue(prop.value);
	}
}

export class StringProperty extends Property {
	value: string;
	isMultiline: boolean;
	public constructor(
		name: string,
		displayName: string,
		value = "",
		isMultiline = false
	) {
		super();
		this.name = name;
		this.displayName = displayName;
		this.value = value;
		this.type = PropertyType.String;
		this.isMultiline = isMultiline;
	}

	public getValue(): any {
		return this.value;
	}

	public setValue(val: any) {
		// todo: validate
		this.value = val;
	}

	public clone(): Property {
		const prop = new StringProperty(this.name, this.displayName, this.value);

		return prop;
	}

	public copyValuesFrom(prop: StringProperty) {
		this.value = prop.value;
	}
}

export class GradientProperty extends Property {
	value: Gradient;
	public constructor(name: string, displayName: string, value: Gradient) {
		super();
		this.name = name;
		this.displayName = displayName;
		this.value = value;
		this.type = PropertyType.Gradient;
	}

	public getValue(): any {
		return this.value;
	}

	public setValue(val: any) {
		console.log("setting gradient value");
		this.value = Gradient.parse(val);
	}

	public clone(): Property {
		const prop = new GradientProperty(
			this.name,
			this.displayName,
			this.value.clone()
		);

		return prop;
	}

	public copyValuesFrom(prop: GradientProperty) {
		console.log("copy value from gradient");
		this.setValue(prop.value.clone());
	}
}
