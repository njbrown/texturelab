<template>
	<div
		class="modal"
		:class="show ? 'show-modal' : ''"
		@click.self="hideModal"
	>
		<div class="menu" ref="menu" @keydown="menuKeyDown">
			<input
				class="search-input"
				type="text"
				placeholder="Filter.."
				v-model="filter"
				ref="search"
				@keydown="preventUpDown"
			/>
			<div class="card-list">
				<div
					v-for="item in filteredList"
					@click="itemClicked(item.type, item.name)"
					:key="item.name"
					class="libcard"
					href="#"
					draggable="true"
					:class="item == selectedItem ? 'selected-card' : ''"
				>
					<img
						v-if="imageExists(item.name)"
						v-bind:src="calcImagePath(item.name)"
						class="thumbnail"
					/>
					<div v-else class="thumbnail" />
					<!-- <span class="thumbnail"></span> -->

					<span class="node-name">{{ item.displayName }}</span>
				</div>
			</div>
		</div>
	</div>
</template>

<script lang="ts">
import { Component, Prop, Vue, Watch } from "vue-property-decorator";
import { DesignerLibrary } from "@/lib/designer/library";
import { LibraryItem, LibraryItemType } from "@/views/Library.vue";
import { Editor } from "@/lib/editortest";
import fs from "fs";
import path from "path";
import { AddItemsAction } from "@/lib/actions/additemsaction";
import { UndoStack } from "@/lib/undostack";

declare var __static: any;

@Component
export default class LibraryMenu extends Vue {
	@Prop() private library!: DesignerLibrary;

	@Prop()
	editor!: Editor;

	mouseX: number = 0;

	mouseY: number = 0;

	filter: string = "";

	show: boolean = false;

	selectedItem: LibraryItem = null;

	mounted() {}

	get items() {
		let items = Object.values(this.library.nodes).map((n) => {
			let item = new LibraryItem(LibraryItemType.Node);
			item.name = n.name;
			item.displayName = n.displayName;

			return item;
		});

		items.push(
			new LibraryItem(LibraryItemType.Comment, "comment", "Comment")
		);
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

	hideModal() {
		this.show = false;
	}

	showModal(x: number, y: number) {
		this.filter = "";
		this.show = true;
		this.selectedItem = null;

		let el = <HTMLElement>this.$refs.menu;
		console.log(x + " " + y);

		el.style.left = x + "px";
		el.style.top = y + "px";

		this.mouseX = x;
		this.mouseY = y;

		//
		window.setTimeout((x) => {
			console.log(<HTMLInputElement>this.$refs["search"]);
			(<HTMLInputElement>this.$refs["search"]).focus();
		}, 0);
	}

	closeModal() {
		this.show = false;
	}

	preventUpDown(evt: KeyboardEvent) {
		if (evt.keyCode == 38 || evt.keyCode == 40) {
			evt.preventDefault();
		}
	}

	addSelectedItem() {
		if (this.selectedItem != null) {
			this.addItem(this.selectedItem.type, this.selectedItem.name);
		}
	}

	// SELECTOR EVENTS
	menuKeyDown(evt: KeyboardEvent) {
		// if enter key, add item
		if (evt.key == "Enter") {
			this.addSelectedItem();
			this.hideModal();
			return;
		}

		// if escape, quit
		if (evt.key == "Escape") {
			this.hideModal();
			return;
		}

		let list = this.filteredList;

		if (list.length == 0) {
			this.selectedItem = null;
			return;
		}

		if (list.length == 1) {
			this.selectedItem = list[0];
			return;
		}

		let index = this.filteredList.indexOf(this.selectedItem);
		//if (index == -1) index = 0; // accept -1 too

		// key up or keydown
		if (evt.keyCode == 38) {
			index -= 1;
		} else if (evt.keyCode == 40) {
			index += 1;
		}

		// clamp
		index = Math.max(0, Math.min(index, list.length - 1));

		this.selectedItem = list[index];
	}

	@Watch("filter")
	updateSelector() {
		// if selected item is not in filtered list
		// select top item

		let list = this.filteredList;

		if (list.length == 0) {
			this.selectedItem = null;
			return;
		}

		if (list.length == 1) {
			this.selectedItem = list[0];
			return;
		}

		let index = this.filteredList.indexOf(this.selectedItem);

		// clamp
		index = Math.max(0, Math.min(index, list.length - 1));

		this.selectedItem = list[index];
	}

	itemClicked(type: LibraryItemType, nodeName: string) {
		this.addItem(type, nodeName);
		this.hideModal();
	}

	addItem(type: LibraryItemType, nodeName: string) {
		let action: AddItemsAction = null;

		// bring mouse pos to local space first
		let canvasPos = this.editor.graph.view.globalToCanvasXY(
			this.mouseX,
			this.mouseY
		);

		let scenePos = this.editor.graph.view.canvasToSceneXY(
			canvasPos.x,
			canvasPos.y
		);

		if (type == LibraryItemType.Node) {
			var dnode = this.library.create(nodeName);
			var canvas = this.editor.canvas;
			var n = this.editor.addNode(
				dnode,
				canvas.width / 2,
				canvas.height / 2
			);

			n.setCenter(scenePos.x, scenePos.y);

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
			item.setCenter(scenePos.x, scenePos.y);

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
			item.setCenter(scenePos.x, scenePos.y);

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
			item.setCenter(scenePos.x, scenePos.y);

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

	imageExists(node: string) {
		//return fs.existsSync(`./public/assets/nodes/${node}.png`);
		return fs.existsSync(path.join(__static, `assets/nodes/${node}.png`));
	}

	calcImagePath(node: string) {
		//return `./assets/nodes/${node}.png`;
		if (process.env.NODE_ENV == "production")
			return (
				"file://" +
				path.join(process.env.BASE_URL, `assets/nodes/${node}.png`)
			);
		return path.join(process.env.BASE_URL, `assets/nodes/${node}.png`);
	}
}
</script>

<style scoped>
/* The Modal (background) */
.modal {
	display: none; /* Hidden by default */
	position: fixed; /* Stay in place */
	z-index: 1000; /* Sit on top */
	left: 0;
	top: 0;
	width: 100%; /* Full width */
	height: 100%; /* Full height */
	overflow: hidden; /* Enable scroll if needed */
	background-color: rgba(0, 0, 0, 0.4); /* Black w/ opacity */
}

.show-modal {
	display: block;
}

/* Modal Content/Box */
.menu {
	position: fixed;
	background-color: #fefefe;
	/* margin: 15% auto; */
	/* padding: 20px; */
	border: 1px solid #888;
	width: 350px; /* Could be more or less, depending on screen size */
	background: rgb(44, 44, 44);
}

.card-list {
	height: 700px;
	overflow-y: auto;
}

.search-input {
	background: transparent;
	color: white;
	border: solid rgb(77, 156, 187) 1px;

	float: none;
	display: block;
	text-align: left;
	width: calc(100% - 20px);
	margin: 10px;
	padding: 10px;
	background: rgb(88, 88, 88);

	box-sizing: border-box;
	font-size: 1.2rem;
}

.search-input:focus {
	outline: none;
}

.thumbnail {
	width: 20px;
}

.libcard {
	padding: 5px;
	color: white;
	border: solid transparent 1px;
	display: table; /* for center-aligning text */
}

.libcard:hover {
	background: rgb(30, 30, 36);
}

.selected-card {
	background: gray;
}

.libcard span {
	padding-left: 10px;
	width: 100%;
	display: table-cell; /* for center-aligning text */
	vertical-align: middle;
}

.libcard img {
	width: 40px;
}
</style>
