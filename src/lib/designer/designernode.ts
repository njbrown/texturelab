import { Guid } from "../utils";
import { Designer, NodeRenderContext } from "../designer";
import {
	Property,
	FloatProperty,
	IntProperty,
	BoolProperty,
	EnumProperty,
	ColorProperty,
	StringProperty,
	GradientProperty,
	IPropertyHolder
} from "./properties";
import { buildShaderProgram } from "./gl";
import { Color } from "./color";
import { Gradient } from "./gradient";

export class NodeInput {
	public node: DesignerNode;
	public name: string;
}

export class DesignerNode implements IPropertyHolder {
	public id: string = Guid.newGuid();
	public title: string;
	public typeName: string; // added when node is created from library

	public gl: WebGLRenderingContext;
	tex: WebGLTexture;
	public designer: Designer;
	exportName: string;

	inputs: string[] = [];
	properties: Property[] = [];

	// tells scene to update the texture next frame
	needsUpdate = true;

	// callbacks
	onthumbnailgenerated: (DesignerNode, HTMLImageElement) => void;

	// an update is requested when:
	// a property is changed
	// a new connection is made
	// a connection is removedscientist
	//
	// all output connected nodes are invalidated as well
	private requestUpdate() {
		this.designer.requestUpdate(this);
	}

	public render(context: NodeRenderContext) {}

	public isCpu() {
		return true;
	}

	public getInputs(): string[] {
		return this.inputs;
	}

	protected addInput(name: string) {
		this.inputs.push(name);
	}

	public setProperty(name: string, value: any) {
		const prop = this.properties.find(x => {
			return x.name == name;
		});

		if (prop) {
			prop.setValue(value);
			this.requestUpdate();
		}

		// for (let prop of this.properties) {
		//   console.log("prop iter");
		//   console.log(prop);
		//   console.log(prop.name == name);
		//   if (prop.name == name) {
		//     prop.setValue(value);
		//     this.requestUpdate();
		//   }
		// }
	}

	public _init() {
		//this.inputs = new Array();
		//this.properties = new Array();
		this.createTexture();

		this.init();
	}

	protected init() {}

	// creates opengl texture for this node
	// gets the height from the scene
	// if the texture is already created, delete it and recreate it
	createTexture() {
		const gl = this.gl as WebGL2RenderingContext;

		if (this.tex) {
			gl.deleteTexture(this.tex);
			this.tex = null;
		}

		const tex = gl.createTexture();
		gl.bindTexture(gl.TEXTURE_2D, tex);

		const level = 0;
		const internalFormat = gl.RGBA16F;
		const border = 0;
		const format = gl.RGBA;
		const type = gl.FLOAT;
		const data = null;
		gl.texImage2D(
			gl.TEXTURE_2D,
			level,
			internalFormat,
			this.designer.width,
			this.designer.height,
			border,
			format,
			type,
			data
		);

		// set the filtering so we don't need mips
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);

		gl.bindTexture(gl.TEXTURE_2D, null);

		this.tex = tex;
	}

	// PROPERTY FUNCTIONS
	addIntProperty(
		id: string,
		displayName: string,
		defaultVal = 1,
		minVal = 1,
		maxVal = 100,
		increment = 1
	): IntProperty {
		const prop = new IntProperty(id, displayName, defaultVal);
		prop.minValue = minVal;
		prop.maxValue = maxVal;
		prop.step = increment;

		this.properties.push(prop);
		return prop;
	}

	addFloatProperty(
		id: string,
		displayName: string,
		defaultVal = 1,
		minVal = 1,
		maxVal = 100,
		increment = 1
	): FloatProperty {
		const prop = new FloatProperty(id, displayName, defaultVal);
		prop.minValue = minVal;
		prop.maxValue = maxVal;
		prop.step = increment;

		this.properties.push(prop);
		return prop;
	}

	addBoolProperty(
		id: string,
		displayName: string,
		defaultVal = false
	): BoolProperty {
		const prop = new BoolProperty(id, displayName, defaultVal);

		this.properties.push(prop);
		return prop;
	}

	addEnumProperty(
		id: string,
		displayName: string,
		defaultVal: string[] = []
	): EnumProperty {
		const prop = new EnumProperty(id, displayName, defaultVal);

		this.properties.push(prop);
		return prop;
	}

	addColorProperty(
		id: string,
		displayName: string,
		defaultVal: Color
	): ColorProperty {
		const prop = new ColorProperty(id, displayName, defaultVal);

		this.properties.push(prop);
		return prop;
	}

	addStringProperty(
		id: string,
		displayName: string,
		defaultVal = ""
	): StringProperty {
		const prop = new StringProperty(id, displayName, defaultVal);

		this.properties.push(prop);
		return prop;
	}

	addGradientProperty(
		id: string,
		displayName: string,
		defaultVal: Gradient
	): GradientProperty {
		const prop = new GradientProperty(id, displayName, defaultVal);

		this.properties.push(prop);
		return prop;
	}
}
