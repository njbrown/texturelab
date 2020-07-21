<template>
	<div class="library-container">
		<div class style="padding-bottom:1em; display:flex;margin:0.5em;">
			<div class="search-container">
				<input type="text" placeholder="Filter.." v-model="filter" />
			</div>
			<!-- <div class="size-container">
        <select>
          <option>Small Icons</option>
          <option>Medium Icons</option>
          <option>Large Icons</option>
          <option>Exra Large Icons</option>
        </select>
			</div>-->
		</div>
		<div class="node-list">
			<div style="overflow:hidden;">
				<span
					v-for="item in filteredList"
					v-on:click="addItem(item.type, item.name)"
					v-on:dragstart="dragStart($event, item)"
					:key="item.name"
					class="libcard"
					href="#"
					draggable="true"
				>
					<!-- <div class="thumbnail" /> -->
					<img
						v-if="imageExists(item.name)"
						v-bind:src="calcImagePath(item.name)"
						class="thumbnail"
					/>
					<div v-else class="thumbnail" />
					<!-- <span class="thumbnail"></span> -->

					<div class="node-name">{{ item.displayName }}</div>
				</span>
			</div>
		</div>
	</div>
</template>

<script lang="ts">
import { Component, Prop, Model, Vue } from "vue-property-decorator";
import { Editor } from "@/lib/editor";
import { DesignerLibrary } from "@/lib/designer/library";
import fs from "fs";
import path from "path";
import { AddItemsAction } from "../lib/actions/additemsaction";
import { UndoStack } from "../lib/undostack";

declare var __static: any;

// this abstracts library nodes and other items such as comments
export enum LibraryItemType {
	Node = "node",
	Comment = "comment",
	Frame = "frame",
	Navigation = "navigation"
}

export class LibraryItem {
	type: LibraryItemType;
	name: string;
	displayName: string;

	constructor(
		type: LibraryItemType,
		name: string = "",
		displayName: string = ""
	) {
		this.type = type;
		this.name = name;
		this.displayName = displayName;
	}
}

@Component
export default class LibraryView extends Vue {
	@Prop()
	library!: DesignerLibrary;

	@Prop()
	editor!: Editor;

	filter: string = "";

	created() {}

	get items() {
		let items = Object.values(this.library.nodes).map(n => {
			let item = new LibraryItem(LibraryItemType.Node);
			item.name = n.name;
			item.displayName = n.displayName;

			return item;
		});

		items.push(new LibraryItem(LibraryItemType.Comment, "comment", "Comment"));
		items.push(new LibraryItem(LibraryItemType.Frame, "frame", "Frame"));
		// items.push(
		// 	new LibraryItem(
		// 		LibraryItemType.Navigation,
		// 		"navigation",
		// 		"Navigation"
		// 	)
		// );

		return items;
	}

	get filteredList() {
		var kw = this.filter;
		var list = Object.values(this.items).filter(function(item) {
			return item.name.toLowerCase().includes(kw.toLowerCase());
		});
		return list;
	}

	addItem(type: LibraryItemType, nodeName: string) {
		let action: AddItemsAction = null;

		if (type == LibraryItemType.Node) {
			var dnode = this.library.create(nodeName);
			var canvas = this.editor.canvas;
			var n = this.editor.addNode(dnode, canvas.width / 2, canvas.height / 2);
			n.setCenter(200, 200);

			action = new AddItemsAction(
				this.editor.graph,
				this.editor.designer,
				[],
				[],
				[],
				[],
				[n],
				[dnode]
			);
		}
		if (type == LibraryItemType.Comment) {
			let item = this.editor.createComment();
			//item.setCenter(200, 200);
			action = new AddItemsAction(
				this.editor.graph,
				this.editor.designer,
				[],
				[item],
				[],
				[],
				[],
				[]
			);
		}
		if (type == LibraryItemType.Frame) {
			let item = this.editor.createFrame();
			//item.setCenter(200, 200);
			action = new AddItemsAction(
				this.editor.graph,
				this.editor.designer,
				[item],
				[],
				[],
				[],
				[],
				[]
			);
		}
		if (type == LibraryItemType.Navigation) {
			let item = this.editor.createNavigation();
			//item.setCenter(200, 200);
			action = new AddItemsAction(
				this.editor.graph,
				this.editor.designer,
				[],
				[],
				[item],
				[],
				[],
				[]
			);
		}

		if (action != null) {
			UndoStack.current.push(action);
		}

		return false;
	}

	dragStart(evt: any, item: any) {
		evt.dataTransfer.setData("text/plain", JSON.stringify(item));
	}

	imageExists(node: string) {
		//return fs.existsSync(`./public/assets/nodes/${node}.png`);
		return fs.existsSync(path.join(__static, `assets/nodes/${node}.png`));
	}

	calcImagePath(node: string) {
		//return `./assets/nodes/${node}.png`;
		if (process.env.NODE_ENV == "production")
			return (
				"file://" + path.join(process.env.BASE_URL, `assets/nodes/${node}.png`)
			);
		return path.join(process.env.BASE_URL, `assets/nodes/${node}.png`);
	}

	mounted() {
		console.log(__static);
		console.log(process.env.BASE_URL);
	}
}
</script>

<style scoped>
.libcard {
	width: 100px;
	display: block;
	float: left;
	padding: 8px;
	cursor: pointer;
	border-radius: 4px;
}

.libcard:hover {
	background: rgb(0, 0, 0, 0.3);
}

.thumbnail {
	display: block;
	width: 100px;
	height: 100px;
	background: #ccc;
	border-radius: 4px;

	margin-left: -4px;
	border: solid rgba(0, 0, 0, 0.7) 4px;
}

.node-name {
	height: 2.6em;
	line-height: 1.3em;
	padding: 0;
	margin: 0;

	color: white;
	text-decoration: none;
}

.node-list {
	overflow-y: scroll;
	flex: 1 1 auto;
}

.search-container {
	flex-grow: 1;
}

.search-container input {
	/* width: calc(100% - 2em); */
	width: calc(100% - 0.5em) !important;
	padding: 0.5em;
	height: 1.5em;
	flex: 0 1 auto;
	padding: 4px;
	margin: 0;
	border-radius: 3px;
	/* border: solid #999 1px; */
	border: 0;
	color: white;
	background: #999;
	outline: none;
}

.size-container {
	flex-grow: 0;
	width: 100px;
}

.size-container select {
	width: 100%;
	height: 100%;
	border: solid white 1px;
	border-radius: 2px;
	color: white;
	background: #222;
	padding: 4px;
}

.library-container {
	overflow: hidden;
	height: 100%;
	box-sizing: border-box;
	display: flex;
	flex-flow: column;
}
</style>
