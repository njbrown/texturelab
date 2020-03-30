import { GraphicsItem } from "./scene/graphicsitem";
import { NodeScene } from "./scene";
import { DesignerLibrary } from "./designer/library";
import { FrameGraphicsItem } from "./scene/framegraphicsitem";
import { Color } from "./designer/color";
import { CommentGraphicsItem } from "./scene/commentgraphicsitem";
import { NavigationGraphicsItem } from "./scene/navigationgraphicsitem";
import { Designer } from "./designer";
import { DesignerNode } from "./designer/designernode";
import { NodeGraphicsItem } from "./scene/nodegraphicsitem";
import { DesignerNodeConn } from "./designer/designerconnection";

export class ItemClipboard {
	public static copyItems(
		designer: Designer,
		library: DesignerLibrary,
		scene: NodeScene,
		clipboard: DataTransfer
	) {
		clipboard.clearData();
		let items = scene.selectedItems;
		if (items.length == 0) {
			// empty clipboard
			clipboard.setData("text/nodes", "");
		}

		let data = {
			nodes: {},
			connections: {},
			comments: [],
			frames: [],
			navigations: [],
			libraryVersion: ""
		};

		// NODES AND CONNECTIONS
		let nodeList: NodeGraphicsItem[] = [];
		items.forEach(i => {
			// check if this works with obfuscated code
			if (i instanceof NodeGraphicsItem)
				nodeList.push(<NodeGraphicsItem>i);
		});

		data.nodes = this.getNodes(designer, nodeList);
		data.connections = this.getConnections(data.nodes, designer, nodeList);

		// FRAMES
		var frames = [];
		for (let item of items) {
			if (!(item instanceof FrameGraphicsItem)) continue;
			let frame = <FrameGraphicsItem>item;

			var n: any = {};
			n["x"] = frame.left;
			n["y"] = frame.top;
			n["width"] = frame.getWidth();
			n["height"] = frame.getHeight();

			n["title"] = frame.title;
			n["showTitle"] = frame.showTitle;
			n["description"] = frame.description;
			n["color"] = frame.color.toHex();

			frames.push(n);
		}
		data.frames = frames;

		// COMMENTS
		var comments = [];
		for (let item of items) {
			if (!(item instanceof CommentGraphicsItem)) continue;
			let comment = <CommentGraphicsItem>item;

			var n: any = {};
			n["x"] = comment.left;
			n["y"] = comment.top;

			n["text"] = comment.text;
			n["color"] = comment.color.toHex();

			comments.push(n);
		}
		data.comments = comments;

		// NAVIGATIONS
		var navs = [];
		for (let item of items) {
			if (!(item instanceof NavigationGraphicsItem)) continue;
			let nav = <NavigationGraphicsItem>item;

			var n: any = {};
			n["x"] = nav.left;
			n["y"] = nav.top;

			navs.push(n);
		}
		data.navigations = navs;

		// let data = scene.save(); // do to items
		data.libraryVersion = library.getVersionName();

		let json = JSON.stringify(data);
		console.log(data);

		clipboard.setData("json/nodes", json);
	}

	public static pasteItems(
		designer: Designer,
		library: DesignerLibrary,
		scene: NodeScene,
		clipboard: DataTransfer
	) {
		//return; // not done!
		let json = clipboard.getData("json/nodes");
		console.log(json);
		if (json == null || json == "") return;

		let data = JSON.parse(json);
		if (!data) return;

		// FRAMES
		if (data.frames) {
			for (let d of data.frames) {
				let frame = new FrameGraphicsItem(scene.view);
				frame.setPos(d.x, d.y);
				frame.setSize(d.width, d.height);

				frame.setTitle(d.title);
				frame.setShowTitle(d.showTitle);
				frame.setDescription(d.description);
				frame.color = Color.parse(d.color);

				scene.addFrame(frame);
			}
		}

		// COMMENTS
		if (data.comments) {
			for (let d of data.comments) {
				let comment = new CommentGraphicsItem(scene.view);
				comment.setPos(d.x, d.y);
				comment.setText(d.text);
				comment.color = Color.parse(d.color);

				scene.addComment(comment);
			}
		}

		// NAVIGATION
		if (data.navigations) {
			for (let d of data.navigations) {
				let nav = new NavigationGraphicsItem();
				nav.setPos(d.x, d.y);
				scene.addNavigation(nav);
			}
		}

		//NODES AND CONNECTIONS
	}

	// merge designer and scene node in one
	// scene node only has x and y values
	static getNodes(designer: Designer, items: NodeGraphicsItem[]): {} {
		let dnodes = {};
		items.forEach(i => {
			let node = designer.getNodeById(i.id);

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

			n["x"] = i.left;
			n["y"] = i.top;

			dnodes[i.id] = n;
		});

		return dnodes;
	}

	static getConnections(
		nodeList: {},
		designer: Designer,
		items: NodeGraphicsItem[]
	): {} {
		let conns = {};

		// we're searching for connections with both left and right socket
		// in our selection pool
		designer.conns.forEach(con => {
			if (
				nodeList.hasOwnProperty(con.leftNode.id) &&
				nodeList.hasOwnProperty(con.rightNode.id)
			) {
				var c = {};
				c["id"] = con.id;
				c["leftNodeId"] = con.leftNode.id;
				c["leftNodeOutput"] = con.leftNodeOutput;
				c["rightNodeId"] = con.rightNode.id;
				c["rightNodeInput"] = con.rightNodeInput;

				conns[con.id] = c;
			}
		});

		return conns;
	}
}
