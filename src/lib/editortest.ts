//import * as scene from "./scene";
import { Designer } from "./designer";
import { DesignerNode } from "./designer/designernode";
import { Property, PropertyType } from "./designer/properties";
import { Guid } from "./utils";
import { DesignerVariableType } from "./designer/designervariable";
import { DesignerLibrary } from "./designer/library";
import { NodeScene } from "./scene";
import { ConnectionGraphicsItem } from "./scene/connectiongraphicsitem";
import { NodeGraphicsItem } from "./scene/nodegraphicsitem";
import { SocketType } from "./scene/socketgraphicsitem";
import { ImageCanvas } from "./designer/imagecanvas";

import { createLibrary as createV1Library } from "@/lib/library/libraryv1";
import { Color } from "./designer/color";
import { CommentGraphicsItem } from "./scene/commentgraphicsitem";
import { FrameGraphicsItem } from "./scene/framegraphicsitem";
import { NavigationGraphicsItem } from "./scene/navigationgraphicsitem";
import { ItemClipboard } from "./clipboard";
import { UndoStack } from "./undostack";
import { RemoveItemsAction } from "./actions/removeitemsaction";

function hexToRgb(hex) {
	var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
	return result
		? {
				r: parseInt(result[1], 16),
				g: parseInt(result[2], 16),
				b: parseInt(result[3], 16),
		  }
		: null;
}

function rgbToHex(r, g, b) {
	return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
}

// stores the IDs for the display nodes
export class DisplayNodes {
	public albedoNode: string;
	public normalNode: string;
	public roughnessNode: string;
	public heightNode: string;
	public metallicNode: string;
	/*
    public albedoCanvas : ImageCanvas = new ImageCanvas();
    public normalCanvas : ImageCanvas = new ImageCanvas();
    public roughnessCanvas : ImageCanvas = new ImageCanvas();
    public heightCanvas : ImageCanvas = new ImageCanvas();
    public metallicCanvas : ImageCanvas = new ImageCanvas();

    resize(width:number, height:number)
    {
        this.albedoCanvas.resize(width, height);
        this.normalCanvas.resize(width, height);
        this.roughnessCanvas.resize(width, height);
        this.heightCanvas.resize(width, height);
        this.metallicCanvas.resize(width, height);
    }
    */
}

export enum DisplayChannel {
	Albedo,
	Metallic,
	Roughness,
	Normal,
	Height,
}

export class Editor {
	canvas: HTMLCanvasElement;

	library: DesignerLibrary;
	graph: NodeScene;
	designer: Designer;
	selectedDesignerNode: DesignerNode;

	undoStack: UndoStack;

	preview2D: HTMLCanvasElement;
	preview2DCtx: CanvasRenderingContext2D;

	scene3D: any; // todo: set a type

	//   propGen: PropertyGenerator;
	//   varGen: VariableGenerator;
	displayNodes: DisplayNodes;

	onnodeselected?: (item: DesignerNode) => void;
	oncommentselected?: (item: CommentGraphicsItem) => void;
	onframeselected?: (item: FrameGraphicsItem) => void;
	onnavigationselected?: (item: NavigationGraphicsItem) => void;
	onpreviewnode?: (item: DesignerNode, image: HTMLCanvasElement) => void;
	onlibrarymenu?: () => void;

	textureChannels = {};
	ontexturechannelcleared?: (
		imageCanvas: ImageCanvas,
		channelName: string
	) => void;
	ontexturechannelassigned?: (
		imageCanvas: ImageCanvas,
		channelName: string
	) => void;
	ontexturechannelupdated?: (
		imageCanvas: ImageCanvas,
		channelName: string
	) => void;

	constructor() {
		this.displayNodes = new DisplayNodes();
		this.selectedDesignerNode = null;
		this.undoStack = new UndoStack();
	}

	getImageWidth() {
		return this.designer.width;
	}

	getImageHeight() {
		return this.designer.height;
	}

	assignNodeToTextureChannel(nodeId: string, channelName: string) {
		// only one node can be assigned to a channel
		if (
			this.textureChannels.hasOwnProperty(channelName) &&
			this.textureChannels[channelName]
		) {
			// remove label from node view
			let oldNode = this.textureChannels[channelName] as DesignerNode;
			let nodeView = this.graph.getNodeById(oldNode.id);
			nodeView.clearTextureChannel();
			//this.textureChannels[channelName] = null;
			delete this.textureChannels[channelName];

			if (this.ontexturechannelcleared) {
				this.ontexturechannelcleared(null, channelName);
			}
		}

		let nodeView = this.graph.getNodeById(nodeId);
		nodeView.setTextureChannel(channelName);

		let newNode = this.designer.getNodeById(nodeId);
		this.textureChannels[channelName] = newNode;

		// notify 3d view
		if (this.ontexturechannelcleared) {
			this.ontexturechannelassigned(nodeView.imageCanvas, channelName);
		}
	}

	clearTextureChannel(nodeId: string) {
		// eval which channel has this node assigned
		for (let channelName in this.textureChannels) {
			let node = this.textureChannels[channelName];

			if (node.id == nodeId) {
				let oldNode = this.textureChannels[channelName] as DesignerNode;
				let nodeView = this.graph.getNodeById(oldNode.id);

				// if this function is called when a node is deleted
				// nodeView will be null
				if (nodeView) nodeView.clearTextureChannel();

				delete this.textureChannels[channelName];

				if (this.ontexturechannelcleared) {
					this.ontexturechannelcleared(null, channelName);
				}
			}
		}

		// only one node can be assigned to a channel
		// if (this.textureChannels.hasOwnProperty(channelName)) {
		//   // remove label from node view
		//   let oldNode = this.textureChannels[channelName] as DesignerNode;
		//   let nodeView = this.graph.getNodeById(oldNode.id);
		//   nodeView.clearTextureChannel();
		//   this.textureChannels[channelName] = null;

		//   if (this.ontexturechannelcleared) {
		//     this.ontexturechannelcleared(oldNode, channelName);
		//   }
		// }
	}

	hasTextureChannel(channelName: string) {
		return this.textureChannels.hasOwnProperty(channelName);
	}

	clearTextureChannels() {
		for (let channelName in this.textureChannels) {
			let node = this.textureChannels[channelName];

			this.clearTextureChannel(node.id);
		}
	}

	getChannelCanvasImage(channelName: string) {
		if (this.hasTextureChannel(channelName)) {
			//console.log(this.textureChannels[channelName]);
			let dnodeId = this.textureChannels[channelName].id;
			let nodeView = this.graph.getNodeById(dnodeId);
			//console.log(nodeView)
			//console.log(this.graph)
			return nodeView.imageCanvas;
		}

		return null;
	}

	/*
    constructor(canvas:HTMLCanvasElement, preview2D:HTMLCanvasElement, propHolder : HTMLElement, varHolder : HTMLElement, scene3D:any)
    {
        this.canvas = canvas;

        this.displayNodes = new DisplayNodes();

        this.preview2D = preview2D;
        this.preview2DCtx = preview2D.getContext("2d");

        this.scene3D = scene3D;
        this.selectedDesignerNode = null;

        this.propGen = new PropertyGenerator(this, propHolder);
        this.varGen = new VariableGenerator(this, varHolder);
        

        //this.setDesigner(new Designer());
        //this.setScene(new NodeScene(canvas));
    }
    */

	undo() {
		this.undoStack.undo();
	}

	redo() {
		this.undoStack.redo();
	}

	// creates new texture
	// requires canvas to be already set
	createNewTexture() {
		this.clearTextureChannels();

		this.library = createV1Library();
		this.setDesigner(new Designer());
		this.setScene(new NodeScene(this.canvas));

		this.setupDefaultScene();
	}

	setupDefaultScene() {
		let offset = 100;
		let spacing = 150;

		// albedo
		let node = this.library.create("output");
		let nodeView = this.addNode(node, 0, 0);
		// figure out why this doesnt work before adding addNode:
		node.setProperty("color", new Color(1, 1, 1, 1));
		nodeView.setCenter(800, offset + spacing * 0);
		console.log(nodeView);
		this.assignNodeToTextureChannel(nodeView.id, "albedo");

		// normal
		node = this.library.create("output");
		nodeView = this.addNode(node, 0, 0);
		node.setProperty("color", new Color(0.5, 0.5, 1, 1));
		nodeView.setCenter(800, offset + spacing * 1);
		this.assignNodeToTextureChannel(nodeView.id, "normal");
		let normalId = node.id;

		// normal map
		node = this.library.create("normalmap");
		nodeView = this.addNode(node, 0, 0);
		nodeView.setCenter(600, offset + spacing * 1);

		this.graph.createConnection(node.id, normalId, 0);

		// roughness
		node = this.library.create("output");
		nodeView = this.addNode(node, 0, 0);
		node.setProperty("color", new Color(0.5, 0.5, 0.5, 1));
		nodeView.setCenter(800, offset + spacing * 2);
		this.assignNodeToTextureChannel(nodeView.id, "roughness");

		// metalness
		node = this.library.create("output");
		nodeView = this.addNode(node, 0, 0);
		node.setProperty("color", new Color(0, 0, 0, 1));
		nodeView.setCenter(800, offset + spacing * 3);
		this.assignNodeToTextureChannel(nodeView.id, "metalness");

		// height
		node = this.library.create("output");
		nodeView = this.addNode(node, 0, 0);
		node.setProperty("color", new Color(0, 0, 0, 1));
		nodeView.setCenter(800, offset + spacing * 4);
		this.assignNodeToTextureChannel(nodeView.id, "height");

		// refresh everything
		this.designer.invalidateAllNodes();
		console.log("default scene setup");
	}

	set2DPreview(preview2D: HTMLCanvasElement) {
		this.preview2D = preview2D;
		this.preview2DCtx = preview2D.getContext("2d");
	}

	setSceneCanvas(canvas: HTMLCanvasElement) {
		this.canvas = canvas;
		this.setScene(new NodeScene(canvas));
	}

	resizeScene(width: number, height: number) {
		this.canvas.width = width;
		this.canvas.height = height;
	}

	set3DScene(scene3D: any) {
		this.scene3D = scene3D;
	}

	setDesigner(designer: Designer) {
		this.designer = designer;
		var self = this;

		designer.onnodetextureupdated = function(dnode) {
			var graphNode = self.graph.getNodeById(dnode.id);
			if (!graphNode) return; // node could have been deleted

			self.designer.copyNodeTextureToImageCanvas(
				dnode,
				graphNode.imageCanvas
			);

			if (self.onpreviewnode) {
				if (dnode == self.selectedDesignerNode)
					self.onpreviewnode(dnode, graphNode.imageCanvas.canvas);
			}

			if (self.ontexturechannelupdated && graphNode.textureChannel) {
				self.ontexturechannelupdated(
					graphNode.imageCanvas,
					graphNode.textureChannel
				);
			}
			// if(node == self.selectedDesignerNode) {
			//     requestAnimationFrame(function(){
			//         self.preview2DCtx.clearRect(0,0,self.preview2D.width, self.preview2D.height);
			//         self.preview2DCtx.drawImage(graphNode.imageCanvas.canvas,
			//             0,0,
			//         self.preview2D.width, self.preview2D.height);
			//     });
			// }

			self.updateDisplayNode(graphNode);
		};

		/*
        designer.onthumbnailgenerated = function(node, thumb) {
            console.log(self.selectedDesignerNode);
            console.log("onthumbnailgenerated generated for: "+node.title);
            // refresh right node image
            var graphNode = self.graph.getNodeById(node.id);
            graphNode.setThumbnail(thumb);
            self.updateDisplayNode(graphNode);

            if(node == self.selectedDesignerNode) {
                requestAnimationFrame(function(){
                    self.preview2DCtx.clearRect(0,0,self.preview2D.width, self.preview2D.height);
                    self.preview2DCtx.drawImage(thumb,
                        0,0,
                    self.preview2D.width, self.preview2D.height);
                });

                
            }
        }
        */

		//if (this.varGen) this.varGen.setDesigner(designer);
		//this.propGen.setDesigner(designer);
	}

	setScene(scene: NodeScene) {
		// cleanup previous graph
		if (this.graph) this.graph.dispose();

		this.undoStack = new UndoStack();
		UndoStack.current = this.undoStack;

		this.graph = scene;

		var self = this;
		this.graph.onconnectioncreated = function(con: ConnectionGraphicsItem) {
			// get node from graph
			var leftNode = con.socketA.node;
			var rightNode = con.socketB.node;

			// get node from designer and connect them
			var leftDNode = self.designer.getNodeById(leftNode.id);
			var rightDNode = self.designer.getNodeById(rightNode.id);

			// make connection
			// switch from `title` to `name`
			self.designer.addConnection(
				leftDNode,
				rightDNode,
				con.socketB.title
			);

			// refresh right node image
			//var thumb = self.designer.generateImageFromNode(rightDNode);
			//rightNode.setThumbnail(thumb);
		};

		this.graph.onconnectiondestroyed = function(
			con: ConnectionGraphicsItem
		) {
			// get node from graph
			var leftNode = con.socketA.node;
			var rightNode = con.socketB.node;

			// get node from designer and connect them
			var leftDNode = self.designer.getNodeById(leftNode.id);
			var rightDNode = self.designer.getNodeById(rightNode.id);

			// remove connection
			// switch from `title` to `name`
			self.designer.removeConnection(
				leftDNode,
				rightDNode,
				con.socketB.title
			);

			// clear right node image
			rightNode.setThumbnail(null);
		};

		this.graph.onnodeselected = function(node: NodeGraphicsItem) {
			if (node != null) {
				var dnode = self.designer.getNodeById(node.id);
				self.selectedDesignerNode = dnode;
				//console.log(dnode);

				if (true) {
					if (self.preview2DCtx) {
						self.preview2DCtx.drawImage(
							node.imageCanvas.canvas,
							0,
							0,
							self.preview2D.width,
							self.preview2D.height
						);
					}

					// todo: move to double click
					if (self.onpreviewnode) {
						self.onpreviewnode(dnode, node.imageCanvas.canvas);
					}

					//console.log(this.scene3D);
					if (self.scene3D) {
						//console.log("setting height texture");
						//self.scene3D.setHeightTexture(node.thumbnail);
						self.updateDisplayNode(node);
					}
				}
			}

			if (self.onnodeselected) self.onnodeselected(dnode);
		};

		this.graph.oncommentselected = function(item: CommentGraphicsItem) {
			if (self.oncommentselected) self.oncommentselected(item);
		};

		this.graph.onframeselected = function(item: FrameGraphicsItem) {
			if (self.onframeselected) self.onframeselected(item);
		};

		this.graph.onnavigationselected = function(
			item: NavigationGraphicsItem
		) {
			if (self.onnavigationselected) self.onnavigationselected(item);
		};

		this.graph.onnodedeleted = function(node: NodeGraphicsItem) {
			// remove node from channels
			//console.log(self);
			self.clearTextureChannel(node.id);

			self.designer.removeNode(node.id);

			if (self.onpreviewnode) {
				self.onpreviewnode(null, null);
			}
		};

		this.graph.onitemsdeleting = function(
			frames: FrameGraphicsItem[],
			comments: CommentGraphicsItem[],
			navs: NavigationGraphicsItem[],
			cons: ConnectionGraphicsItem[],
			nodes: NodeGraphicsItem[]
		) {
			let dnodes: DesignerNode[] = [];
			for (let node of nodes) {
				let dnode = self.designer.getNodeById(node.id);

				// should never happen!
				if (dnode == null)
					throw "Node with id " + dnode.id + " doesnt exist!!";

				dnodes.push(dnode);
			}

			let action = new RemoveItemsAction(
				self,
				self.graph,
				self.designer,
				frames,
				comments,
				navs,
				cons,
				nodes,
				dnodes
			);
			UndoStack.current.push(action);
		};

		this.graph.onitemsdeleted = function(
			frames: FrameGraphicsItem[],
			comments: CommentGraphicsItem[],
			navs: NavigationGraphicsItem[],
			cons: ConnectionGraphicsItem[],
			nodes: NodeGraphicsItem[]
		) {
			if (self.onpreviewnode) {
				self.onpreviewnode(null, null);
			}
		};

		this.graph.oncopy = function(evt: ClipboardEvent) {
			self.executeCopy(evt);
		};

		this.graph.oncut = function(evt: ClipboardEvent) {
			self.executeCut(evt);
		};

		this.graph.onpaste = function(evt: ClipboardEvent) {
			self.executePaste(evt);
		};

		this.graph.onlibrarymenu = function() {
			console.log(self.onlibrarymenu);
			if (self.onlibrarymenu != null) {
				self.onlibrarymenu();
			}
		};

		// property changes
		/*
        this.propGen.onnodepropertychanged = function(dnode:DesignerNode, prop:Property) {
            //var node = self.graph.getNodeById(node.id);
            //self.graph.refreshNode()
            
            // todo: do this properly
            var thumb = self.designer.generateImageFromNode(dnode);
            var node = self.graph.getNodeById(dnode.id);
            node.thumbnail = thumb;

            //console.log(node.thumbnail);
            requestAnimationFrame(function(){
                self.preview2DCtx.clearRect(0,0,self.preview2D.width, self.preview2D.height);
                self.preview2DCtx.drawImage(thumb,
                    0,0,
                self.preview2D.width, self.preview2D.height);
            });
            
            // just a stest
            //self.scene3D.setHeightTexture(node.thumbnail);
            self.updateDisplayNode(node);
        }
        */
	}

	executeCopy(evt: ClipboardEvent) {
		ItemClipboard.copyItems(
			this.designer,
			this.library,
			this.graph,
			evt.clipboardData
		);
	}

	executeCut(evt: ClipboardEvent) {
		ItemClipboard.copyItems(
			this.designer,
			this.library,
			this.graph,
			evt.clipboardData
		);
	}

	executePaste(evt: ClipboardEvent) {
		ItemClipboard.pasteItems(
			this.designer,
			this.library,
			this.graph,
			evt.clipboardData
		);
	}

	// adds node
	// x and y are screen space
	addNode(
		dNode: DesignerNode,
		screenX: number = 0,
		screenY: number = 0
	): NodeGraphicsItem {
		// must add to designer first
		this.designer.addNode(dNode);

		// create node from designer
		var node = new NodeGraphicsItem(dNode.title);
		for (let input of dNode.getInputs()) {
			node.addSocket(input, input, SocketType.In);
		}
		node.addSocket("output", "output", SocketType.Out);
		this.graph.addNode(node);
		node.id = dNode.id;

		// generate thumbnail
		var thumb = this.designer.generateImageFromNode(dNode);
		node.setThumbnail(thumb);

		var pos = this.graph.view.canvasToSceneXY(screenX, screenY);
		node.setCenter(pos.x, pos.y);

		return node;
	}

	createComment(): CommentGraphicsItem {
		let comment = new CommentGraphicsItem(this.graph.view);
		var pos = this.graph.view.sceneCenter;
		comment.setCenter(pos.x, pos.y);

		this.graph.addComment(comment);

		return comment;
	}

	createFrame(): FrameGraphicsItem {
		let frame = new FrameGraphicsItem(this.graph.view);
		var pos = this.graph.view.sceneCenter;
		frame.setCenter(pos.x, pos.y);

		this.graph.addFrame(frame);

		return frame;
	}

	createNavigation(): NavigationGraphicsItem {
		let nav = new NavigationGraphicsItem();
		var pos = this.graph.view.sceneCenter;
		nav.setCenter(pos.x, pos.y);

		this.graph.addNavigation(nav);

		return nav;
	}

	// DISPLAY NODE FUNCTIONS

	// updates appropriate image if set
	updateDisplayNode(node: NodeGraphicsItem) {
		if (!this.scene3D) return;

		//console.log(node.id);
		//console.log(this.displayNodes.normalNode);

		// TODO: create custom CanvasImage that resizes with
		// the texture size. NodeGraphicsItem's CanvasImage is fixed
		// to 1024x1024. Another option is to give each DesignerNode a
		// CanvasImage that updates when its texture updates then pass
		// it to NodeGraphicsitem. That way it gets used one place and
		// gets updated everywhere else all at once.
		if (node.id == this.displayNodes.albedoNode) {
			//this.scene3D.setAlbedoTexture(node.thumbnail);
			this.scene3D.setAlbedoCanvasTexture(node.imageCanvas.canvas);
		}

		if (node.id == this.displayNodes.metallicNode) {
			//this.scene3D.setMetallicTexture(node.thumbnail);
			this.scene3D.setMetallicCanvasTexture(node.imageCanvas.canvas);
		}

		if (node.id == this.displayNodes.normalNode) {
			//this.scene3D.setNormalTexture(node.thumbnail);
			this.scene3D.setNormalCanvasTexture(node.imageCanvas.canvas);
		}

		if (node.id == this.displayNodes.roughnessNode) {
			//this.scene3D.setRoughnessTexture(node.thumbnail);
			this.scene3D.setRoughnessCanvasTexture(node.imageCanvas.canvas);
		}

		if (node.id == this.displayNodes.heightNode) {
			//this.scene3D.setHeightTexture(node.thumbnail);
			this.scene3D.setHeightCanvasTexture(node.imageCanvas.canvas);
		}
	}

	setDisplayChannelNode(channel: DisplayChannel, nodeId: string) {
		var node = this.graph.getNodeById(nodeId);
		if (channel == DisplayChannel.Albedo) {
			this.displayNodes.albedoNode = nodeId;
		}
		if (channel == DisplayChannel.Metallic) {
			this.displayNodes.metallicNode = nodeId;
		}
		if (channel == DisplayChannel.Normal) {
			this.displayNodes.normalNode = nodeId;
		}
		if (channel == DisplayChannel.Roughness) {
			this.displayNodes.roughnessNode = nodeId;
		}
		if (channel == DisplayChannel.Height) {
			this.displayNodes.heightNode = nodeId;
		}

		this.updateDisplayNode(node);
	}

	exposeVariable(node: DesignerNode, prop: Property, varDisplayName: string) {
		// create new variable
		var varName = Guid.newGuid();
		var dvar = this.designer.addVariable(
			varName,
			varDisplayName,
			this.evalDesignerVariableType(prop)
		);
		// copy over important props
		// todo:make more elegant
		dvar.property = prop.clone();
		dvar.property.name = varName;
		dvar.property.displayName = varDisplayName;

		// add it to scene and bind prop
		this.designer.mapNodePropertyToVariable(varName, node, prop.name);

		// copy property props

		// refresh var ui
		// this.varGen.refreshUi();
	}

	evalDesignerVariableType(prop: Property): DesignerVariableType {
		if (prop.type == PropertyType.Float) {
			return DesignerVariableType.Float;
		} else if (prop.type == PropertyType.Int) {
			return DesignerVariableType.Int;
		} else if (prop.type == PropertyType.Bool) {
			return DesignerVariableType.Bool;
		} else if (prop.type == PropertyType.Enum) {
			return DesignerVariableType.Enum;
		} else if (prop.type == PropertyType.Color) {
			return DesignerVariableType.Color;
		} else {
			console.log("error: invalid property type for variable", prop);
		}

		return null;
	}

	update() {
		if (this.designer) this.designer.update();
	}

	draw() {
		if (this.graph) this.graph.draw();
	}

	load(data: any) {
		// clear texture channels
		this.clearTextureChannels();

		let library;
		if (!data["libraryVersion"]) {
			library = createV1Library();
		} else {
			// library = this.createLibrary(data["libraryVersion"]);
			library = createV1Library();
		}
		// load scene
		var d = Designer.load(data, library);

		// load graph
		var g = NodeScene.load(d, data["scene"], this.canvas);

		//todo: properly destroy existing graph

		//this.designer = d;
		//this.graph = g;
		this.setDesigner(d);
		this.setScene(g);

		// assign each node to it's texture channel
		// it's expected at this point that the 3d preview should already
		// have the texturechannel callbacks assigned

		// load editor data
		if (data["editor"] != null) {
			var e = data["editor"];
			// console.log("loading editor data");
			// console.log(e.displayNodes);

			// this.displayNodes.albedoNode = e.displayNodes.albedoNode;
			// this.displayNodes.metallicNode = e.displayNodes.metallicNode;
			// this.displayNodes.normalNode = e.displayNodes.normalNode;
			// this.displayNodes.roughnessNode = e.displayNodes.roughnessNode;
			// this.displayNodes.heightNode = e.displayNodes.heightNode;

			for (let channelName in e.textureChannels) {
				if (!e.textureChannels.hasOwnProperty(channelName)) continue;
				console.log(e);
				let node = this.graph.getNodeById(
					e.textureChannels[channelName]
				);
				if (node) this.assignNodeToTextureChannel(node.id, channelName);
			}

			//this.textureChannels = e.textureChannels || {};
			//console.log(this.textureChannels)
		}
	}

	save(): any {
		var data = this.designer.save();
		data["scene"] = this.graph.save();

		let textureChannels = {};
		for (let channelName in this.textureChannels) {
			textureChannels[channelName] = this.textureChannels[channelName].id;
		}

		data["editor"] = {
			//displayNodes: this.displayNodes,
			textureChannels: textureChannels,
		};

		//data["libraryVersion"] = this.library.getVersionName();
		data["libraryVersion"] = "v1";

		return data;
	}
}
