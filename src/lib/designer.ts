import { DesignerNode, NodeInput } from "./designer/designernode";
import { DesignerNodeConn } from "./designer/designerconnection";
import { DesignerLibrary } from "./designer/library";
import { Guid } from "./utils";
import { ImageCanvas } from "./designer/imagecanvas";
import { buildShaderProgram } from "./designer/gl";
import {
	IntProperty,
	FloatProperty,
	BoolProperty,
	EnumProperty,
	ColorProperty,
	Property,
} from "./designer/properties";
import { Color } from "./designer/color";
import {
	DesignerVariable,
	DesignerVariableType,
	DesignerNodePropertyMap,
} from "./designer/designervariable";

export class Designer {
	canvas: HTMLCanvasElement;
	gl: WebGLRenderingContext;
	public texCoordBuffer: WebGLBuffer;
	public posBuffer: WebGLBuffer;
	vertexShaderSource: string;
	fbo: WebGLFramebuffer;
	thumbnailProgram: WebGLProgram;

	randomSeed: number;
	width: number;
	height: number;

	nodes: DesignerNode[];
	conns: DesignerNodeConn[];

	// list of nodes yet to be designed
	updateList: DesignerNode[];

	library: DesignerLibrary;

	//variables
	variables: DesignerVariable[];

	// callbacks
	onthumbnailgenerated: (DesignerNode, HTMLImageElement) => void;

	// called everytime a node's texture gets updated
	// listeners can use this update their CanvasTextures
	// by rendering the node's texture with renderNodeTextureToCanvas(node, imageCanvas)
	onnodetextureupdated: (DesignerNode) => void;

	public constructor() {
		this.width = 1024;
		this.height = 1024;
		this.randomSeed = 32;

		this.canvas = <HTMLCanvasElement>document.createElement("canvas");
		//document.body.appendChild(this.canvas);
		this.canvas.width = this.width;
		this.canvas.height = this.height;
		this.gl = this.canvas.getContext("webgl2");

		this.nodes = new Array();
		this.conns = new Array();

		this.updateList = new Array();
		this.variables = new Array();
		this.init();
	}

	public setTextureSize(width: number, height: number) {
		//todo: is resizing the canvas even necessary?
		this.width = width;
		this.height = height;
		this.canvas.width = this.width;
		this.canvas.height = this.height;

		for (let node of this.nodes) {
			node.createTexture();
			this.requestUpdate(node);
		}
	}

	public randomizeSeed() {
		this.setRandomSeed(Math.floor(Math.random() * 256));
	}

	public setRandomSeed(newSeed: number) {
		this.randomSeed = newSeed;
		// invalidate all nodes
		this.invalidateAllNodes();
	}

	public getRandomSeed(): number {
		return this.randomSeed;
	}

	init() {
		this.createVertexBuffers();
		this.createFBO();
		this.createThumbmailProgram();
	}

	update() {
		var updateQuota = 10000000000000;
		// fetch random node from update list (having all in sockets that have been updated) and update it
		// todo: do only on per update loop
		while (this.updateList.length != 0) {
			for (let node of this.updateList) {
				if (this.haveAllUpdatedLeftNodes(node)) {
					// update this node's texture and thumbnail

					// a note about this:
					// technically all the child nodes should be updated here, so this.updateList
					// wont be touched in this function
					// so we avoid messing up our loop since the length of this.updateList wont change
					this.generateImageFromNode(node);

					// remove from list
					this.updateList.splice(this.updateList.indexOf(node), 1);
					node.needsUpdate = false;
					//break;// one per update loop

					updateQuota--;
					if (updateQuota < 0) break;
				}
			}
		}
	}

	// checks if all input nodes have needsUpdate set to false
	haveAllUpdatedLeftNodes(node: DesignerNode): boolean {
		for (let con of this.conns) {
			// get connections to this node
			if (con.rightNode == node) {
				if (con.leftNode.needsUpdate == true) {
					// found a node that needs update itself
					return false;
				}
			}
		}

		return true;
	}

	// adds node to update list
	// add subsequent (output) nodes in tree to update list a well, recursively
	requestUpdate(node: DesignerNode) {
		if (this.updateList.indexOf(node) == -1) {
			// not yet in the list, add to list and add dependent nodes
			node.needsUpdate = true; // just in case...
			this.updateList.push(node);
		}

		// add all right connections
		for (let con of this.conns) {
			if (con.leftNode == node) {
				this.requestUpdate(con.rightNode);
			}
		}
	}

	public invalidateAllNodes() {
		for (let node of this.nodes) {
			this.requestUpdate(node);
		}
	}

	setLibrary(lib: DesignerLibrary) {
		this.library = lib;
	}

	// creates node and adds it to scene
	createNode(name: string): DesignerNode {
		var node = this.library.create(name);

		this.addNode(node);
		return node;
	}

	createFBO() {
		var gl = this.gl;

		this.fbo = gl.createFramebuffer();

		// gotta create at least a renderbuffer
	}

	createVertexBuffers() {
		var gl = this.gl;
		//var texCoordLocation = gl.getAttribLocation(program, "a_texCoord");

		// provide texture coordinates for the rectangle.
		this.texCoordBuffer = gl.createBuffer();
		gl.bindBuffer(gl.ARRAY_BUFFER, this.texCoordBuffer);
		gl.bufferData(
			gl.ARRAY_BUFFER,
			new Float32Array([
				0.0,
				0.0,
				1.0,
				0.0,
				0.0,
				1.0,
				0.0,
				1.0,
				1.0,
				0.0,
				1.0,
				1.0,
			]),
			gl.STATIC_DRAW
		);
		//gl.enableVertexAttribArray(texCoordLocation);
		//gl.vertexAttribPointer(texCoordLocation, 2, gl.FLOAT, false, 0, 0);

		this.posBuffer = gl.createBuffer();
		gl.bindBuffer(gl.ARRAY_BUFFER, this.posBuffer);
		gl.bufferData(
			gl.ARRAY_BUFFER,
			new Float32Array([
				-1.0,
				-1.0,
				0.0,
				1.0,
				-1.0,
				0.0,
				-1.0,
				1.0,
				0.0,
				-1.0,
				1.0,
				0.0,
				1.0,
				-1.0,
				0.0,
				1.0,
				1.0,
				0.0,
			]),
			gl.STATIC_DRAW
		);

		gl.bindBuffer(gl.ARRAY_BUFFER, null);
	}

	// adds node to scene
	// adds it to this.updateList
	// if `init` is set to false, node will not be initialized
	// useful for undo-redo where node is re-added
	addNode(node: DesignerNode, init: boolean = true) {
		this.nodes.push(node);
		if (init) {
			node.gl = this.gl;
			node.designer = this;
			node._init();
		}
		this.requestUpdate(node);
	}

	getNodeById(nodeId: string): DesignerNode {
		for (let node of this.nodes) {
			if (node.id == nodeId) return node;
		}
		return null;
	}

	addConnection(
		leftNode: DesignerNode,
		rightNode: DesignerNode,
		rightIndex: string
	) {
		let con = new DesignerNodeConn();
		con.leftNode = leftNode;

		con.rightNode = rightNode;
		con.rightNodeInput = rightIndex;

		this.conns.push(con);

		// right node needs to be updated
		this.requestUpdate(rightNode);

		return con;
	}

	removeConnection(
		leftNode: DesignerNode,
		rightNode: DesignerNode,
		rightIndex: string
	) {
		for (let con of this.conns) {
			if (
				con.leftNode == leftNode &&
				con.rightNode == rightNode &&
				con.rightNodeInput == rightIndex
			) {
				// right node needs to be updated
				this.requestUpdate(rightNode);

				// found our connection, remove
				this.conns.splice(this.conns.indexOf(con), 1);
				//console.log("removed connection in designer");
				//console.log(con);

				return con;
			}
		}

		return null;
	}

	// todo: double check connections just in case
	removeNode(nodeId: string): DesignerNode {
		let node = this.getNodeById(nodeId);
		if (!node) {
			return null;
		}

		this.nodes.splice(this.nodes.indexOf(node), 1);

		// it's safe here to pluck this node right out of the update queue
		// the connections would have been already removed, triggering
		// updates for the previously neighbor nodes already
		while (this.updateList.indexOf(node) !== -1)
			this.updateList.splice(this.updateList.indexOf(node), 1);
	}

	generateImage(name: string): HTMLImageElement {
		var node: DesignerNode = this.getNodeByName(name);
		return this.generateImageFromNode(node);
	}

	// this function generates the image of the node given its input nodes
	// if the input nodes arent updated then it will update them
	// for every node updated in this function, it emits onthumbnailgenerated(node, thumbnail)
	// it returns a thumbnail (an html image)

	generateImageFromNode(node: DesignerNode): HTMLImageElement {
		//console.log("generating node "+node.exportName);
		// process input nodes
		var inputs: NodeInput[] = this.getNodeInputs(node);
		for (let input of inputs) {
			if (input.node.needsUpdate) {
				this.generateImageFromNode(input.node);

				// remove from update list since thumbnail has now been generated
				input.node.needsUpdate = false;
				this.updateList.splice(this.updateList.indexOf(input.node), 1);
			}
		}

		var gl = this.gl;

		// todo: move to node maybe
		gl.bindFramebuffer(gl.FRAMEBUFFER, this.fbo);
		gl.activeTexture(gl.TEXTURE0);
		gl.framebufferTexture2D(
			gl.FRAMEBUFFER,
			gl.COLOR_ATTACHMENT0,
			gl.TEXTURE_2D,
			node.tex,
			0
		);

		gl.viewport(0, 0, this.width, this.height);
		node.render(inputs);

		gl.bindFramebuffer(gl.FRAMEBUFFER, null);

		if (this.onnodetextureupdated) {
			this.onnodetextureupdated(node);
		}

		var thumb = this.generateThumbnailFromNode(node);
		if (this.onthumbnailgenerated) {
			this.onthumbnailgenerated(node, thumb);
		}

		return thumb;
	}

	// renders node's texture to an image object
	// ensure the node is updated before calling this function
	// this function doesnt try to update child nodes
	generateThumbnailFromNode(node: DesignerNode) {
		var gl = this.gl;

		gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

		// bind shader
		gl.useProgram(this.thumbnailProgram);

		// bind mesh
		var posLoc = gl.getAttribLocation(this.thumbnailProgram, "a_pos");
		var texCoordLoc = gl.getAttribLocation(
			this.thumbnailProgram,
			"a_texCoord"
		);

		// provide texture coordinates for the rectangle.
		gl.bindBuffer(gl.ARRAY_BUFFER, this.posBuffer);
		gl.enableVertexAttribArray(posLoc);
		gl.vertexAttribPointer(posLoc, 3, gl.FLOAT, false, 0, 0);

		gl.bindBuffer(gl.ARRAY_BUFFER, this.texCoordBuffer);
		gl.enableVertexAttribArray(texCoordLoc);
		gl.vertexAttribPointer(texCoordLoc, 2, gl.FLOAT, false, 0, 0);

		// send texture
		gl.uniform1i(gl.getUniformLocation(this.thumbnailProgram, "tex"), 0);
		gl.activeTexture(gl.TEXTURE0);
		gl.bindTexture(gl.TEXTURE_2D, node.tex);

		gl.drawArrays(gl.TRIANGLES, 0, 6);

		// cleanup
		gl.disableVertexAttribArray(posLoc);
		gl.disableVertexAttribArray(texCoordLoc);

		//var img:HTMLImageElement = <HTMLImageElement>document.createElement("image");
		//var img:HTMLImageElement = new Image(this.width, this.height);
		//img.src = this.canvas.toDataURL("image/png");

		// note: this called right after clears the image for some reason
		//gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
		//return img;
		return null;
	}

	// render's node's texture then draws it on the given canvas
	// used as an alternative to move textures since toDataUrl is
	// so computationally expensive
	copyNodeTextureToImageCanvas(node: DesignerNode, canvas: ImageCanvas) {
		var gl = this.gl;

		gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

		// bind shader
		gl.useProgram(this.thumbnailProgram);

		// bind mesh
		var posLoc = gl.getAttribLocation(this.thumbnailProgram, "a_pos");
		var texCoordLoc = gl.getAttribLocation(
			this.thumbnailProgram,
			"a_texCoord"
		);

		// provide texture coordinates for the rectangle.
		gl.bindBuffer(gl.ARRAY_BUFFER, this.posBuffer);
		gl.enableVertexAttribArray(posLoc);
		gl.vertexAttribPointer(posLoc, 3, gl.FLOAT, false, 0, 0);

		gl.bindBuffer(gl.ARRAY_BUFFER, this.texCoordBuffer);
		gl.enableVertexAttribArray(texCoordLoc);
		gl.vertexAttribPointer(texCoordLoc, 2, gl.FLOAT, false, 0, 0);

		// send texture
		gl.uniform1i(gl.getUniformLocation(this.thumbnailProgram, "tex"), 0);
		gl.activeTexture(gl.TEXTURE0);
		gl.bindTexture(gl.TEXTURE_2D, node.tex);

		gl.drawArrays(gl.TRIANGLES, 0, 6);

		// cleanup
		gl.disableVertexAttribArray(posLoc);
		gl.disableVertexAttribArray(texCoordLoc);

		// force rendering to be complete
		//gl.flush();

		canvas.copyFromCanvas(this.canvas, true);
	}

	createThumbmailProgram() {
		var prog = buildShaderProgram(
			this.gl,
			`precision mediump float;

        attribute vec3 a_pos;
        attribute vec2 a_texCoord;
            
        // the texCoords passed in from the vertex shader.
        varying vec2 v_texCoord;
            
        void main() {
            gl_Position = vec4(a_pos,1.0);
            v_texCoord = a_texCoord;
        }`,
			`precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D tex;

        vec4 process(vec2 uv);
        
        void main() {
            gl_FragColor = texture2D(tex,v_texCoord);
        }`
		);

		this.thumbnailProgram = prog;
	}

	getNodeByName(exportName: string): DesignerNode {
		for (let node of this.nodes) {
			if (node.exportName == exportName) return node;
		}

		return null;
	}

	getNodeInputs(node: DesignerNode): NodeInput[] {
		var inputs: NodeInput[] = new Array();

		for (let con of this.conns) {
			if (con.rightNode == node) {
				let input = new NodeInput();
				input.name = con.rightNodeInput;
				input.node = con.leftNode;
				inputs.push(input);
			}
		}

		return inputs;
	}

	public addVariable(
		name: string,
		displayName: string,
		varType: DesignerVariableType
	): DesignerVariable {
		//todo: throw exception if variable already exists?

		var variable = new DesignerVariable();
		variable.type = varType;
		variable.id = Guid.newGuid();

		switch (varType) {
			case DesignerVariableType.Int:
				variable.property = new IntProperty(name, displayName, 0);
				break;
			case DesignerVariableType.Float:
				variable.property = new FloatProperty(name, displayName, 0);
				break;
			case DesignerVariableType.Bool:
				variable.property = new BoolProperty(name, displayName, false);
				break;
			case DesignerVariableType.Enum:
				variable.property = new EnumProperty(name, displayName, []);
				break;
			case DesignerVariableType.Color:
				variable.property = new ColorProperty(
					name,
					displayName,
					new Color()
				);
				break;
		}

		this.variables.push(variable);
		return variable;
	}

	// todo: keep reference inside node's property
	public mapNodePropertyToVariable(
		varName: string,
		node: DesignerNode,
		nodePropName: string
	) {
		var variable = this.findVariable(varName);
		if (variable == null) return; //todo: throw exception?

		var map = new DesignerNodePropertyMap();
		map.node = node;
		map.propertyName = nodePropName;

		variable.nodes.push(map);
	}

	//todo: remove property map

	public setVariable(name: string, value: any) {
		var variable = this.findVariable(name);
		if (variable) {
			//todo: throw exception for invalid types being set
			variable.property.setValue(value);

			//update each node's variables
			for (let nodeMap of variable.nodes) {
				//if (nodeMap.node.hasProperty(nodeMap.propertyName))// just incase
				nodeMap.node.setProperty(nodeMap.propertyName, value);
			}
		} else {
			// throw exception?
		}
	}

	public findVariable(name: string) {
		for (let variable of this.variables)
			if (variable.property.name == name) return variable;
		return null;
	}

	public hasVariable(name: string): boolean {
		for (let variable of this.variables)
			if (variable.property.name == name) return true;
		return false;
	}

	public variableCount(): number {
		return this.variables.length;
	}

	public save(): any {
		var nodes = new Array();
		for (let node of this.nodes) {
			var n = {};
			n["id"] = node.id;
			n["typeName"] = node.typeName;
			n["exportName"] = node.exportName;
			//n["inputs"] = node.inputs;// not needed imo

			var props = {};
			for (let prop of node.properties) {
				props[prop.name] = prop.getValue();
			}
			n["properties"] = props;

			nodes.push(n);
		}

		var connections = new Array();
		for (let con of this.conns) {
			var c = {};
			c["id"] = con.id;
			c["leftNodeId"] = con.leftNode.id;
			c["leftNodeOutput"] = con.leftNodeOutput;
			c["rightNodeId"] = con.rightNode.id;
			c["rightNodeInput"] = con.rightNodeInput;

			connections.push(c);
		}

		var variables = new Array();
		for (let dvar of this.variables) {
			var v = {};
			v["id"] = dvar.id;
			v["type"] = dvar.type;
			v["property"] = dvar.property;

			var nodeIds = new Array();
			for (let n of dvar.nodes) {
				nodeIds.push({
					nodeId: n.node.id,
					name: n.propertyName,
				});
			}
			v["linkedProperties"] = nodeIds;
			variables.push(v);
			console.log(v);
		}

		var data = {};
		data["nodes"] = nodes;
		data["connections"] = connections;
		data["variables"] = variables;
		return data;
	}

	static load(data: any, lib: DesignerLibrary): Designer {
		console.log(data);
		var d = new Designer();
		var nodes = data["nodes"];
		for (let node of nodes) {
			var n = lib.create(node["typeName"]);
			n.exportName = node["exportName"];
			n.id = node["id"];

			// add node to it's properties will be initialized
			// todo: separate setting properties and inputs from setting shader in node
			d.addNode(n);

			// add properties
			var properties = node["properties"];
			for (var prop in properties) {
				n.setProperty(prop, properties[prop]);
			}
		}

		var connections = data["connections"];
		for (let con of connections) {
			//var c = d.addConnection()
			var left = d.getNodeById(con.leftNodeId);
			var right = d.getNodeById(con.rightNodeId);

			// todo: support left index
			d.addConnection(left, right, con.rightNodeInput);
		}
		/*
        for(let dvar of this.variables) {
            var v = {};
            v["id"] = dvar.id;
            v["type"] = dvar.type;
            v["property"] = dvar.property;

            var nodeIds = new Array();
            for(let n of dvar.nodes) {
                nodeIds.push({
                    nodeId:n.node.id,
                    name:n.propertyName
                });
            }
            v["linkedProperties"] = nodeIds;
            variables.push(v);
            console.log(v);
        }
        */
		if (data.variables) {
			var variables = <DesignerVariable[]>data.variables;
			for (let v of variables) {
				//this.addVariable(v.name, v.displayName, )

				var dvar = d.addVariable(
					v.property.name,
					v.property.displayName,
					v.type
				);
				dvar.id = v.id;
				// copy values over to the property
				switch (dvar.type) {
					case DesignerVariableType.Float:
						(<FloatProperty>dvar.property).copyValuesFrom(<
							FloatProperty
						>v.property);
						break;

					case DesignerVariableType.Int:
						(<IntProperty>dvar.property).copyValuesFrom(<
							IntProperty
						>v.property);
						break;

					case DesignerVariableType.Bool:
						(<BoolProperty>dvar.property).copyValuesFrom(<
							BoolProperty
						>v.property);
						break;

					case DesignerVariableType.Enum:
						(<EnumProperty>dvar.property).copyValuesFrom(<
							EnumProperty
						>v.property);
						break;

					case DesignerVariableType.Color:
						(<ColorProperty>dvar.property).copyValuesFrom(<
							ColorProperty
						>v.property);
						break;
				}

				// link properties
				for (let lp of (<any>v).linkedProperties) {
					let node = d.getNodeById(lp.nodeId);
					d.mapNodePropertyToVariable(v.property.name, node, lp.name);
				}
			}
		}

		return d;
	}
}
