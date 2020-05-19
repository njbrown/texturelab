import {
	GraphicsItem,
	MouseDownEvent,
	MouseMoveEvent,
	MouseUpEvent,
} from "./graphicsitem";
import {
	NodeGraphicsItem,
	NodeGraphicsItemRenderState,
} from "./nodegraphicsitem";
import { ConnectionGraphicsItem } from "./connectiongraphicsitem";
import { AddConnectionAction } from "../actions/addconnectionaction";
import { UndoStack } from "../undostack";
import { RemoveConnectionAction } from "../actions/removeconnectionaction";
import {
	ConnectionSwitchAction,
	SwitchConnectionAction,
} from "../actions/switchconnectionaction";

export enum SocketType {
	In,
	Out,
}

export class SocketGraphicsItem extends GraphicsItem {
	public id!: string;
	public title!: string;
	public node!: NodeGraphicsItem;
	public socketType!: SocketType;
	radius: number = 8;

	// only in sockets store the connection
	// since outsockets can have multiple connections
	//public con:ConnectionGraphicsItem;
	conns: ConnectionGraphicsItem[] = new Array();

	hit: boolean;
	hitSocket: SocketGraphicsItem;
	hitConnection: ConnectionGraphicsItem; // for removal
	mouseDragX: number;
	mouseDragY: number;

	addConnection(con: ConnectionGraphicsItem) {
		this.conns.push(con);
	}
	removeConnection(con: ConnectionGraphicsItem) {
		this.conns.splice(this.conns.indexOf(con), 1);
	}
	getConnection(index: number): ConnectionGraphicsItem {
		return this.conns[index];
	}

	// retruns a connection where the outSocket == socketA
	// returns null if no result is found
	getConnectionFrom(socketA: SocketGraphicsItem) {
		for (let con of this.conns) {
			if (con.socketA == socketA) return con;
		}

		return null;
	}

	// retruns a connection where the inSocket == socketB
	// returns null if no result is found
	getConnectionTo(socketB: SocketGraphicsItem) {
		for (let con of this.conns) {
			if (con.socketB == socketB) return con;
		}

		return null;
	}

	hasConnections() {
		return this.conns.length > 0;
	}

	constructor() {
		super();
		this.width = this.radius * 2;
		this.height = this.radius * 2;

		this.hit = false;
		this.hitSocket = null;
	}

	draw(ctx: CanvasRenderingContext2D, renderData: any = null) {
		const renderState = <NodeGraphicsItemRenderState>renderData;

		//console.log(this.width);
		ctx.lineWidth = 3;
		//ctx.rect(this.x, this.y, this.width, this.height);
		ctx.beginPath();
		ctx.fillStyle = "rgb(150,150,150)";
		ctx.arc(this.centerX(), this.centerY(), this.radius, 0, 2 * Math.PI);
		ctx.fill();

		// border
		ctx.beginPath();
		ctx.arc(this.centerX(), this.centerY(), this.radius, 0, 2 * Math.PI);
		ctx.strokeStyle = "rgb(0, 0, 0)";
		ctx.stroke();

		// draw inner dot if connected
		if (this.hasConnections()) {
			//console.log("con");
			ctx.beginPath();
			ctx.fillStyle = "rgb(100,100,100)";
			ctx.arc(
				this.centerX(),
				this.centerY(),
				this.radius / 3,
				0,
				2 * Math.PI
			);
			ctx.fill();
		}

		this.drawActiveConnection(ctx);

		// draw text
		if (renderState.hovered) {
			ctx.fillStyle = "rgb(150,150,150)";
			ctx.font = "9px 'Open Sans'";
			if (this.socketType == SocketType.Out) {
				let w = ctx.measureText(this.title).width;
				ctx.fillText(this.title, this.x + this.width + 4, this.y + 12);
			} else {
				let w = ctx.measureText(this.title).width;
				ctx.fillText(this.title, this.x - 4 - w, this.y + 12);
			}
		}
	}

	drawActiveConnection(ctx: CanvasRenderingContext2D) {
		if (this.hitSocket) {
			let mouseX = this.mouseDragX;
			let mouseY = this.mouseDragY;

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
		}
	}

	// MOUSE EVENTS
	public mouseDown(evt: MouseDownEvent) {
		this.hit = true;
		this.hitSocket = null;
		this.mouseDragX = evt.globalX;
		this.mouseDragY = evt.globalY;

		// if socket is an in socket with a connection, make hitsocket the connected out socket
		if (this.socketType == SocketType.In && this.hasConnections()) {
			this.hitSocket = this.getConnection(0).socketA; // insockets should only have one connection
			// store connection for removal as well
			this.hitConnection = this.getConnection(0);
			console.log("hit connection");
			console.log(this.hitConnection);
		} else this.hitSocket = this;
	}

	public mouseMove(evt: MouseMoveEvent) {
		if (this.hit) {
			// movement
			//this.move(evt.deltaX, evt.deltaY);
			this.mouseDragX = evt.globalX;
			this.mouseDragY = evt.globalY;
		}
	}

	public mouseUp(evt: MouseUpEvent) {
		console.log("mouse up!!");
		let mouseX = evt.globalX;
		let mouseY = evt.globalY;

		// for undo-redo
		let movedCons: ConnectionGraphicsItem[] = [];
		let actions: ConnectionSwitchAction[] = [];

		if (this.hitSocket) {
			// remove previous connection
			// this block creates a new connection regardless of the outcome
			if (this.hitConnection) {
				this.scene.removeConnection(this.hitConnection);

				// let action = new RemoveConnectionAction(
				// 	this.scene,
				// 	this.hitConnection
				// );
				// UndoStack.current.push(action);

				movedCons.push(this.hitConnection);
				actions.push(ConnectionSwitchAction.Remove);

				this.hitConnection = null;
			}

			let closeSock: SocketGraphicsItem = this.scene.getHitSocket(
				mouseX,
				mouseY
			);

			if (
				closeSock &&
				closeSock != this.hitSocket &&
				closeSock.socketType != this.hitSocket.socketType &&
				closeSock.node != this.hitSocket.node
			) {
				// close socket
				var con: ConnectionGraphicsItem = new ConnectionGraphicsItem();
				// out socket should be on the left, socketA
				if (this.hitSocket.socketType == SocketType.Out) {
					// out socket
					con.socketA = this.hitSocket;
					con.socketB = closeSock;

					// close sock is an inSocket which means it should only have one connection
					// remove current connection from inSocket
					if (closeSock.hasConnections()) {
						let removeCon = closeSock.getConnection(0);
						this.scene.removeConnection(removeCon);
						movedCons.push(removeCon);
						actions.push(ConnectionSwitchAction.Remove);

						// let action = new RemoveConnectionAction(
						// 	this.scene,
						// 	removeCon
						// );
						// UndoStack.current.push(action);
					}
				} else {
					// in socket
					con.socketA = closeSock;
					con.socketB = this.hitSocket;
				}

				// link connection
				//con.socketA.con = con;
				//con.socketB.con = con;

				this.scene.addConnection(con);
				movedCons.push(con);
				actions.push(ConnectionSwitchAction.Add);

				// let action = new AddConnectionAction(this.scene, con);
				// UndoStack.current.push(action);
			} else if (!closeSock) {
				// delete connection if hit node is an insock
				// if we're here it means one of 2 things:
				// 1: a new connection failed to form
				// 2: we're breaking a previously formed connection, which can only be done
				// by dragging from an insock that already has a connection

				if (this.hitSocket.socketType == SocketType.Out) {
					/*
			                if (this.hitSocket.hasConnections()) {
			                    // remove connection
			                    //let con = this.hitSocket.con;
			                    this.removeConnection(this.hitSocket.getConnectionFrom(this.hitSocket));
			                }
			                */

					if (this.hitConnection) {
						this.scene.removeConnection(this.hitConnection);

						movedCons.push(this.hitConnection);
						actions.push(ConnectionSwitchAction.Remove);
						// let action = new RemoveConnectionAction(
						// 	this.scene,
						// 	this.hitConnection
						// );
						// UndoStack.current.push(action);
					}
				}
			}
		}

		this.hit = false;
		this.hitSocket = null;
		this.hitConnection = null;

		// undo-redo
		let action = new SwitchConnectionAction(this.scene, movedCons, actions);
		UndoStack.current.push(action);
	}
}
