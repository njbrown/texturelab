<template>
	<div class="field">
		<div>
			<label>{{ prop.displayName }}</label>
		</div>
		<div class="input-holder">
			<canvas @click="loadImage()" width="150" height="150" ref="canvas" />
			<div class="image-buttons">
				<button class="image-button load-button" @click="loadImage()">
					load
				</button>
				<button class="image-button reload-button" @click="reloadImage()">
					reload
				</button>
				<button class="image-button remove-button" @click="removeImage()">
					remove
				</button>
			</div>
		</div>
	</div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit, Ref } from "vue-property-decorator";
import { Designer } from "@/lib/designer";
import { DesignerNode } from "@/lib/designer/designernode";
import { ImageProperty, IPropertyHolder } from "../../lib/designer/properties";
import { PropertyChangeComplete } from "./ipropertyui";
import { UndoStack } from "@/lib/undostack";
import { PropertyChangeAction } from "@/lib/actions/propertychangeaction";
import { Image } from "@/lib/designer/image";
import * as fs from "fs";

const electron = require("electron");
const remote = require("@electron/remote");
const { dialog, app, BrowserWindow, Menu } = remote;

@Component
export default class ImagePropertyView extends Vue {
	@Prop()
	prop: any; // HTMLImagelElement

	@Prop()
	designer: Designer;

	@Prop()
	propHolder: IPropertyHolder;

	@Ref("canvas")
	canvas: HTMLCanvasElement;

	val: Image = null;

	mounted() {
		this.val = this.prop.value;
		this.renderImageToCanvas();
	}

	@Emit()
	propertyChanged() {
		return this.prop.name;
	}

	@Emit()
	propertyChangeCompleted(evt: PropertyChangeComplete) {
		return evt;
	}

	undoUpdate() {
		this.val = this.prop.value;
		this.renderImageToCanvas();
	}

	loadImage() {
		let paths = dialog.showOpenDialogSync(remote.getCurrentWindow(), {
			filters: [
				{
					name: "Supported Images",
					extensions: ["png", "jpg"]
				}
			],
			defaultPath: "image"
		});

		if (!paths || paths.length == 0) return;

		this.loadImageFromPath(paths[0]);
	}

	reloadImage() {
		let path = null;
		if (this.val && this.val.path) {
			path = this.val.path;
		}

		if (path === null) {
			return;
		}

		if (fs.existsSync(path)) {
			this.loadImageFromPath(path);
		} else {
			alert("Image file doesn't exist: '" + path + "'");
		}
	}

	private loadImageFromPath(filePath: string) {
		let path = "image://" + filePath;
		let img: HTMLImageElement = document.createElement(
			"img"
		) as HTMLImageElement;

		img.onload = () => {
			console.log("image loaded");

			// flip image
			let canvas = document.createElement("canvas");
			canvas.width = img.width;
			canvas.height = img.height;
			let ctx = canvas.getContext("2d");
			ctx.save();
			ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
			ctx.translate(0, img.height);
			ctx.scale(1, -1);
			ctx.drawImage(img, 0, 0, img.width, img.height);
			ctx.restore();

			// create image object
			// let image = new Image(path, img, img.width, img.height);
			let image = new Image(filePath, canvas, canvas.width, canvas.height);

			let action = new PropertyChangeAction(
				() => {
					this.undoUpdate();
				},
				this.prop.name,
				this.propHolder,
				this.val,
				image
			);
			UndoStack.current.push(action);

			this.val = image;
			this.propHolder.setProperty(this.prop.name, image);
			this.renderImageToCanvas();
		};

		img.src = path;
	}

	removeImage() {
		let image = Image.empty();

		let action = new PropertyChangeAction(
			() => {
				this.undoUpdate();
			},
			this.prop.name,
			this.propHolder,
			this.val,
			image
		);
		UndoStack.current.push(action);

		this.val = image;
		this.propHolder.setProperty(this.prop.name, image);
		this.renderImageToCanvas();
	}

	renderImageToCanvas() {
		requestAnimationFrame(() => {
			console.log(this);

			if (!this.$refs.canvas) return;

			let ctx = (this.$refs.canvas as HTMLCanvasElement).getContext("2d");
			let c = this.val.canvas;
			ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

			if (this.val == null) return;

			if (this.val.isEmpty) return;

			ctx.save();
			ctx.translate(0, this.canvas.height);
			ctx.scale(1, -1);
			ctx.drawImage(c, 0, 0, this.canvas.width, this.canvas.height);
			ctx.restore();
		});
	}
}
</script>

<style scoped>
.field {
	font-size: 12px;
	padding: 0.9em 0.5em;
	color: rgba(255, 255, 255, 0.7);
	border-bottom: 1px rgb(61, 61, 61) solid;
}

.field label {
	font-weight: bold;
	padding: 0.4em;
	padding-left: 0;
}

.texture-options {
	background: #e0e0e0;
	border-radius: 3px;
	margin-bottom: 1em !important;
	padding: 1em;
}

.input-holder {
	display: flex;
}

.image-buttons {
	display: flex;
	flex-direction: column;
	margin-left: 5px;
}

.image-button {
	width: 48px;
	height: 48px;
	margin: 1px;
	box-sizing: border-box;
	background: #222;
	border: none;
	color: white;
	padding: 0;
	cursor: pointer;
	border-radius: 4px;
}

.image-button:hover {
	background: rgb(88, 88, 88);
}
</style>
