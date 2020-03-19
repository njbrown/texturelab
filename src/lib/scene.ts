import { ImageCanvas } from "./designer/imagecanvas";
import { Designer } from "./designer";
import {
	NodeGraphicsItem,
	NodeGraphicsItemRenderState
} from "./scene/nodegraphicsitem";
import { ConnectionGraphicsItem } from "./scene/connectiongraphicsitem";
import { SocketGraphicsItem, SocketType } from "./scene/socketgraphicsitem";
import {
	GraphicsItem,
	MouseMoveEvent,
	MouseDownEvent,
	MouseUpEvent
} from "./scene/graphicsitem";
import { SceneView } from "./scene/view";
import { FrameGraphicsItem } from "./scene/framegraphicsitem";
import { CommentGraphicsItem } from "./scene/commentgraphicsitem";
import { NavigationGraphicsItem } from "./scene/navigationgraphicsitem";

enum DragMode {
	None,
	Selection,
	Nodes,
	Socket,
	Frame,
	Pin,
	Comment
}

class Selection {
	nodes: NodeGraphicsItem[];

	public clear() {
		this.nodes = [];
	}
}
class Rect {
	x: number;
	y: number;
	width: number;
	height: number;

	Rect(x: number = 0, y: number = 0, width: number = 1, height: number = 1) {
		this.x = x;
		this.y = y;
		this.width = width;
		this.height = height;
	}
}

export class NodeScene {
	canvas: HTMLCanvasElement;
	context!: CanvasRenderingContext2D;
	contextExtra: any;
	hasFocus: boolean;

	frames: FrameGraphicsItem[];
	comments: CommentGraphicsItem[];
	navigations: NavigationGraphicsItem[];
	nodes: NodeGraphicsItem[];
	conns: ConnectionGraphicsItem[];

	dragMode: DragMode;
	selectionRect: Rect;
	draggedNode?: NodeGraphicsItem;
	selectedNode: NodeGraphicsItem;
	hitSocket?: SocketGraphicsItem;
	hitConnection?: ConnectionGraphicsItem;
	selectedItem: GraphicsItem;
	hitItem: GraphicsItem;

	// callbacks
	onconnectioncreated?: (item: ConnectionGraphicsItem) => void;
	onconnectiondestroyed?: (item: ConnectionGraphicsItem) => void;
	// passes null if no node is selected
	onnodeselected?: (item: NodeGraphicsItem) => void;
	oncommentselected?: (item: CommentGraphicsItem) => void;
	onframeselected?: (item: FrameGraphicsItem) => void;
	onnavigationselected?: (item: NavigationGraphicsItem) => void;

	onnodedeleted?: (item: NodeGraphicsItem) => void;

	view: SceneView;

	constructor(canvas: HTMLCanvasElement) {
		this.canvas = canvas;
		this.context = this.canvas.getContext("2d");
		this.view = new SceneView(canvas);
		this.hasFocus = false;
		this.contextExtra = this.context;
		this.frames = new Array();
		this.comments = new Array();
		this.nodes = new Array();
		this.conns = new Array();
		this.navigations = new Array();
		this.dragMode = null;
		this.selectionRect = new Rect();
		this.selectedItem = null;
		this.hitItem = null;

		// add sample frames
		let frame = new FrameGraphicsItem(this.view);
		frame.setSize(500, 300);
		frame.scene = this;
		this.frames.push(frame);

		let comment = new CommentGraphicsItem(this.view);
		comment.scene = this;
		//comment.setText("Hello World");
		comment.setText("This\nis\na\nmultiline\nmessage");
		comment.setCenter(200, 500);
		this.comments.push(comment);

		let nav = new NavigationGraphicsItem();
		nav.scene = this;
		nav.setLabel("Test Navigation");
		this.navigations.push(nav);

		// bind event listeners
		var self = this;
		canvas.addEventListener("mousemove", function(evt: MouseEvent) {
			self.onMouseMove(evt);
		});
		canvas.addEventListener("mousedown", function(evt: MouseEvent) {
			self.onMouseDown(evt);
		});
		canvas.addEventListener("mouseup", function(evt: MouseEvent) {
			self.onMouseUp(evt);
		});

		window.addEventListener("click", function(evt: MouseEvent) {
			//console.log(evt.target == canvas);
			if (evt.target == canvas) {
				self.hasFocus = true;
			} else {
				self.hasFocus = false;
			}
		});

		window.addEventListener(
			"keydown",
			function(evt: KeyboardEvent) {
				if (evt.key == "Delete" && self.hasFocus && self.selectedNode) {
					self.deleteNode(self.selectedNode);
				}
			},
			true
		);
		// canvas.addEventListener("mousewheel", function(evt: WheelEvent) {
		//   self.onMouseScroll(evt);
		// });
		canvas.addEventListener("contextmenu", function(evt: MouseEvent) {
			evt.preventDefault();
		});
	}

	addNode(item: NodeGraphicsItem) {
		this.nodes.push(item);
	}

	deleteNode(item: NodeGraphicsItem) {
		// delete connections
		let conns = this.conns;
		for (let i = this.conns.length - 1; i >= 0; i--) {
			let con = this.conns[i];
			if (
				(con.socketA && con.socketA.node.id == item.id) ||
				(con.socketB && con.socketB.node.id == item.id)
			) {
				this.removeConnection(con);
			}
		}

		// remove node from list
		this.nodes.splice(this.nodes.indexOf(item), 1);

		// if node is selected (which it most likely is), clear it from selection
		this.selectedNode = null;

		// emit deselection
		if (this.onnodeselected) this.onnodeselected(null);

		// emit remove event
		if (this.onnodedeleted) this.onnodedeleted(item);
	}

	getNodeById(id: string): NodeGraphicsItem {
		for (let node of this.nodes) {
			if (node.id == id) return node;
		}
		return null;
	}

	//todo: integrity check
	addConnection(con: ConnectionGraphicsItem) {
		this.conns.push(con);

		// link the sockets
		con.socketA.addConnection(con);
		con.socketB.addConnection(con);

		// callback
		if (this.onconnectioncreated) this.onconnectioncreated(con);
	}

	createConnection(leftId: string, rightId: string, rightIndex: number = 0) {
		var con = new ConnectionGraphicsItem();

		// get nodes
		var leftNode = this.getNodeById(leftId);
		var rightNode = this.getNodeById(rightId);

		// get sockets
		con.socketA = leftNode.sockets.find(
			x => x.socketType == SocketType.Out
		);
		con.socketB = rightNode.sockets[rightIndex];

		this.addConnection(con);
	}

	removeConnection(con: ConnectionGraphicsItem) {
		console.log("removing connection in scene");
		console.log(con);

		this.conns.splice(this.conns.indexOf(con), 1);
		//con.socketA.con = null;
		//con.socketB.con = null;
		con.socketA.removeConnection(con);
		con.socketB.removeConnection(con);

		// callback
		if (this.onconnectiondestroyed) this.onconnectiondestroyed(con);
	}

	// if the user click drags on a socket then it's making a connection
	drawActiveConnection() {
		let mouse = this.view.getMouseSceneSpace();
		let mouseX = mouse.x;
		let mouseY = mouse.y;

		let ctx = this.context;
		if (this.hitSocket) {
			ctx.beginPath();
			ctx.strokeStyle = "rgb(200, 200, 200)";
			ctx.lineWidth = 4;
			ctx.moveTo(this.hitSocket.centerX(), this.hitSocket.centerY());

			if (this.hitSocket.socketType == SocketType.Out) {
				ctx.bezierCurveTo(
					this.hitSocket.centerX() + 60,
					this.hitSocket.centerY(), // control point 1
					mouseX - 60,
					mouseY,
					mouseX,
					mouseY
				);
			} else {
				ctx.bezierCurveTo(
					this.hitSocket.centerX() - 60,
					this.hitSocket.centerY(), // control point 1
					mouseX + 60,
					mouseY,
					mouseX,
					mouseY
				);
			}

			ctx.setLineDash([5, 3]);
			ctx.stroke();
			ctx.setLineDash([]);

			ctx.beginPath();
			ctx.fillStyle = "rgb(200, 200, 200)";
			const radius = 6;
			ctx.arc(mouseX, mouseY, radius, 0, 2 * Math.PI);
			ctx.fill();
		}
	}

	clearAndDrawGrid() {
		//this.context.scale(2,2);
		// this.context.fillStyle = "rgb(120, 120, 120)";
		// var topCorner = this.view.canvasToSceneXY(0, 0);
		// var bottomCorner = this.view.canvasToSceneXY(
		//   this.canvas.clientWidth,
		//   this.canvas.clientHeight
		// );
		// this.context.fillRect(
		//   topCorner.x,
		//   topCorner.y,
		//   bottomCorner.x - topCorner.x,
		//   bottomCorner.y - topCorner.y
		// );
		//this.context.fillRect(0,0,this.canvas.width, this.canvas.height);

		// todo: draw grid

		this.view.clear(this.context, "#4A5050");
		this.view.setViewMatrix(this.context);
		this.view.drawGrid(this.context, 33.33333, "#4E5454", 1);
		this.view.drawGrid(this.context, 100, "#464C4C", 3);
	}

	draw() {
		this.clearAndDrawGrid();

		// draw frames
		for (let frame of this.frames) frame.draw(this.context);

		// draw comments
		for (let comment of this.comments) comment.draw(this.context);

		// draw connections
		for (let con of this.conns) {
			if (con == this.hitConnection) continue;
			con.draw(this.context);
		}

		if (this.hitSocket) {
			this.drawActiveConnection();
		}

		// draw nodes
		let mouse = this.view.getMouseSceneSpace();
		let mouseX = mouse.x;
		let mouseY = mouse.y;
		let nodeState: NodeGraphicsItemRenderState = {
			hovered: false, // mouse over
			selected: false // selected node
		};
		for (let item of this.nodes) {
			// check for selection ( only do this when not dragging anything )
			if (item == this.selectedNode) nodeState.selected = true;
			else nodeState.selected = false;

			// check for hover
			if (item.isPointInside(mouseX, mouseY) && this.hitSocket == null)
				nodeState.hovered = true;
			else nodeState.hovered = false;

			item.draw(this.context, nodeState);
		}

		for (let nav of this.navigations) nav.draw(this.context);
	}

	// mouse events
	onMouseDown(evt: MouseEvent) {
		var pos = this.getScenePos(evt);
		let mouseX = pos.x;
		let mouseY = pos.y;

		if (evt.button == 0) {
			let hitItem = this.getHitItem(mouseX, mouseY);
			if (hitItem != null) {
				let mouseEvent = new MouseDownEvent();
				mouseEvent.globalX = pos.x;
				mouseEvent.globalY = pos.y;
				mouseEvent.localX = hitItem.left - pos.x;
				mouseEvent.localY = hitItem.top - pos.y;

				hitItem.mouseDown(mouseEvent);
				if (mouseEvent.isAccepted) {
					this.hitItem = hitItem;

					if (hitItem instanceof NodeGraphicsItem) {
						let hitNode = <NodeGraphicsItem>hitItem;
						//move node to stop of stack
						this.moveNodeToTop(hitNode);

						if (this.onnodeselected) {
							if (hitNode) this.onnodeselected(hitNode);
							else this.onnodeselected(hitNode);
						}
					}

					if (hitItem instanceof CommentGraphicsItem) {
						let hitComment = <CommentGraphicsItem>hitItem;

						if (this.oncommentselected) {
							if (hitComment) this.oncommentselected(hitComment);
							else this.oncommentselected(hitComment);
						}
					}

					if (hitItem instanceof FrameGraphicsItem) {
						let hit = <FrameGraphicsItem>hitItem;

						if (this.onframeselected) {
							if (hit) this.onframeselected(hit);
							else this.onframeselected(hit);
						}
					}

					if (hitItem instanceof NavigationGraphicsItem) {
						let hit = <NavigationGraphicsItem>hitItem;

						if (this.onnavigationselected) {
							if (hit) this.onnavigationselected(hit);
							else this.onnavigationselected(hit);
						}
					}
				}
			}

			// check for a hit socket first
			// let hitSock: SocketGraphicsItem = this.getHitSocket(mouseX, mouseY);

			// if (hitSock) {
			// 	// if socket is an in socket with a connection, make hitsocket the connected out socket
			// 	if (
			// 		hitSock.socketType == SocketType.In &&
			// 		hitSock.hasConnections()
			// 	) {
			// 		this.hitSocket = hitSock.getConnection(0).socketA; // insockets should only have one connection
			// 		// store connection for removal as well
			// 		this.hitConnection = hitSock.getConnection(0);
			// 	} else this.hitSocket = hitSock;
			// } else {
			// 	// if there isnt a hit socket then check for a hit node
			// 	let hitNode: NodeGraphicsItem = this.getHitNode(mouseX, mouseY);

			// 	if (hitNode) {
			// 		//move node to stop of stack
			// 		this.moveNodeToTop(hitNode);

			// 		// todo: do this properly on mouse release
			// 		this.selectedNode = hitNode;
			// 	} else {
			// 		this.selectedNode = null;
			// 	}

			// 	this.draggedNode = hitNode;
			// 	if (this.onnodeselected) {
			// 		if (hitNode) this.onnodeselected(hitNode);
			// 		else this.onnodeselected(hitNode);
			// 	}
			// }
		}
	}

	// https://stackoverflow.com/questions/5306680/move-an-array-element-from-one-array-position-to-another
	moveNodeToTop(node: NodeGraphicsItem) {
		var index = this.nodes.indexOf(node);
		if (index === -1) {
			console.log(
				"Attempting to push node that doesnt exist in node list"
			);
		}
		this.nodes.splice(index, 1);
		this.nodes.push(node);
	}

	onMouseUp(evt: MouseEvent) {
		var pos = this.getScenePos(evt);
		let mouseX = pos.x;
		let mouseY = pos.y;

		if (evt.button == 0) {
			if (this.hitItem != null) {
				let hitItem = this.hitItem;

				let mouseEvent = new MouseUpEvent();
				mouseEvent.globalX = pos.x;
				mouseEvent.globalY = pos.y;
				mouseEvent.localX = hitItem.left - pos.x;
				mouseEvent.localY = hitItem.top - pos.y;

				hitItem.mouseUp(mouseEvent);

				this.hitItem = null;
			}
		}

		// if (evt.button == 0) {
		// 	if (this.hitSocket) {
		// 		// remove previous connection
		// 		// this block creates a new connection regardless of the outcome
		// 		if (this.hitConnection) {
		// 			this.removeConnection(this.hitConnection);
		// 			this.hitConnection = null;
		// 		}

		// 		let closeSock: SocketGraphicsItem = this.getHitSocket(
		// 			mouseX,
		// 			mouseY
		// 		);

		// 		if (
		// 			closeSock &&
		// 			closeSock != this.hitSocket &&
		// 			closeSock.socketType != this.hitSocket.socketType &&
		// 			closeSock.node != this.hitSocket.node
		// 		) {
		// 			// close socket
		// 			var con: ConnectionGraphicsItem = new ConnectionGraphicsItem();
		// 			// out socket should be on the left, socketA
		// 			if (this.hitSocket.socketType == SocketType.Out) {
		// 				// out socket
		// 				con.socketA = this.hitSocket;
		// 				con.socketB = closeSock;

		// 				// close sock is an inSocket which means it should only have one connection
		// 				// remove current connection from inSocket
		// 				if (closeSock.hasConnections())
		// 					this.removeConnection(closeSock.getConnection(0));
		// 			} else {
		// 				// in socket
		// 				con.socketA = closeSock;
		// 				con.socketB = this.hitSocket;
		// 			}

		// 			// link connection
		// 			//con.socketA.con = con;
		// 			//con.socketB.con = con;

		// 			this.addConnection(con);
		// 		} else if (!closeSock) {
		// 			// delete connection if hit node is an insock
		// 			// if we're here it means one of 2 things:
		// 			// 1: a new connection failed to form
		// 			// 2: we're breaking a previously formed connection, which can only be done
		// 			// by dragging from an insock that already has a connection

		// 			if (this.hitSocket.socketType == SocketType.Out) {
		// 				/*
		//                 if (this.hitSocket.hasConnections()) {
		//                     // remove connection
		//                     //let con = this.hitSocket.con;
		//                     this.removeConnection(this.hitSocket.getConnectionFrom(this.hitSocket));
		//                 }
		//                 */

		// 				if (this.hitConnection)
		// 					this.removeConnection(this.hitConnection);
		// 			}
		// 		}
		// 	}

		// 	this.draggedNode = null;
		// 	this.hitSocket = null;
		// 	this.hitConnection = null;
		// }
	}

	onMouseMove(evt: MouseEvent) {
		var pos = this.getScenePos(evt);

		if (this.hitItem) {
			let mouseEvent = new MouseMoveEvent();
			mouseEvent.globalX = pos.x;
			mouseEvent.globalY = pos.y;

			mouseEvent.localX = this.hitItem.left - pos.x;
			mouseEvent.localY = this.hitItem.top - pos.y;

			let drag = this.view.getMouseDeltaSceneSpace();
			mouseEvent.deltaX = drag.x;
			mouseEvent.deltaY = drag.y;

			this.hitItem.mouseMove(mouseEvent);
		}

		// // handle dragged socket
		// if (this.hitSocket) {
		// }

		// // handle dragged node
		// if (this.draggedNode != null) {
		// 	//var diff = this.view.canvasToSceneXY(evt.movementX, evt.movementY);
		// 	//console.log("move: ",evt.movementX,evt.movementY);
		// 	//this.draggedNode.move(evt.movementX, evt.movementY);

		// 	// view keeps track of dragging
		// 	let drag = this.view.getMouseDeltaSceneSpace();
		// 	this.draggedNode.move(drag.x, drag.y);
		// }
	}

	// hit detection
	// x and y are scene space
	getHitNode(x: number, y: number): NodeGraphicsItem {
		// for (let node of this.nodes) {
		for (var index = this.nodes.length - 1; index >= 0; index--) {
			let node = this.nodes[index];
			if (node.isPointInside(x, y)) return node;
		}

		return null;
	}

	getHitSocket(x: number, y: number): SocketGraphicsItem {
		for (let node of this.nodes) {
			for (let sock of node.sockets) {
				if (sock.isPointInside(x, y)) return sock;
			}
		}

		return null;
	}

	// gets item over mouse x and y
	// obeys precedence
	getHitItem(x: number, y: number): GraphicsItem {
		// 1) navigation pins
		for (var index = this.navigations.length - 1; index >= 0; index--) {
			let nav = this.navigations[index];

			if (nav.isPointInside(x, y)) return nav;
		}

		// 2) nodes and their sockets
		for (var index = this.nodes.length - 1; index >= 0; index--) {
			let node = this.nodes[index];

			for (let sock of node.sockets) {
				if (sock.isPointInside(x, y)) return sock;
			}

			if (node.isPointInside(x, y)) return node;
		}

		// 3) comments
		for (var index = this.comments.length - 1; index >= 0; index--) {
			let comment = this.comments[index];

			if (comment.isPointInside(x, y)) return comment;
		}

		// 4) frame
		for (var index = this.frames.length - 1; index >= 0; index--) {
			let frame = this.frames[index];

			if (frame.isPointInside(x, y)) return frame;
		}

		return null;
	}

	// UTILITY

	// returns the scene pos from the mouse event
	getScenePos(evt: MouseEvent) {
		var canvasPos = _getMousePos(this.canvas, evt);
		return this.view.canvasToSceneXY(canvasPos.x, canvasPos.y);
	}

	// SAVE/LOAD

	// only save position data to associative array
	save(): any {
		var data: any = {};

		var nodes = {};
		for (let node of this.nodes) {
			var n: any = {};
			n["id"] = node.id;
			n["x"] = node.centerX();
			n["y"] = node.centerY();

			nodes[node.id] = n;
		}
		data["nodes"] = nodes;

		return data;
	}

	static load(
		designer: Designer,
		data: any,
		canvas: HTMLCanvasElement
	): NodeScene {
		var s = new NodeScene(canvas);

		// add nodes one by one
		for (let dNode of designer.nodes) {
			// create node from designer
			var node = new NodeGraphicsItem(dNode.title);
			for (let input of dNode.getInputs()) {
				node.addSocket(input, input, SocketType.In);
			}
			node.addSocket("output", "output", SocketType.Out);
			s.addNode(node);
			node.id = dNode.id;

			// get position
			var x = data["nodes"][node.id].x;
			var y = data["nodes"][node.id].y;
			node.setCenter(x, y);
		}

		// add connection one by one
		for (let dcon of designer.conns) {
			var con = new ConnectionGraphicsItem();
			con.id = dcon.id;

			// get nodes
			var leftNode = s.getNodeById(dcon.leftNode.id);
			var rightNode = s.getNodeById(dcon.rightNode.id);

			// get sockets
			con.socketA = leftNode.getOutSocketByName(dcon.leftNodeOutput);
			con.socketB = rightNode.getInSocketByName(dcon.rightNodeInput);

			s.addConnection(con);
		}

		return s;
	}
}

// https://www.html5canvastutorials.com/advanced/html5-canvas-mouse-coordinates/
// https://stackoverflow.com/questions/17130395/real-mouse-position-in-canvas
function _getMousePos(canvas, evt) {
	var rect = canvas.getBoundingClientRect();
	return {
		x: evt.clientX - rect.left,
		y: evt.clientY - rect.top
	};
}
