<template>
	<div style="height:100%">
		<div style="height:2em;">
			<a
				:class="{ btn: true, toggled: !hasImage }"
				href="#"
				@click="saveTexture()"
				><i class="bx bx-save bx-sm"></i
			></a>
			<a
				:class="{ btn: true, toggled: isTiling }"
				href="#"
				@click="toggleNineTexures()"
				><i class="bx bx-grid bx-sm"></i
			></a>
			<a class="btn" href="#" @click="centerTexture()"
				><i class="bx bx-exit-fullscreen bx-sm"></i
			></a>
		</div>
		<canvas id="_2dpreview" ref="canvas" style="display:block;"></canvas>
	</div>
</template>

<script>
import { DragZoom, DrawMode } from "./preview2d/previewcanvas2d";
import { unobserve } from "../unobserve";
const electron = require("electron");
const remote = electron.remote;
const { dialog, app, BrowserWindow, Menu } = remote;
import fs from "fs";
var nativeImage = electron.nativeImage;

export default {
	// props: {
	//   editor: {
	//     type: Object
	//   }
	// },
	data() {
		return {
			node: null,
			// HtmlCanvasElement...it will auto update when the node's image changes
			image: null,
			canvas: null,
			dragZoom: null
		};
	},
	mounted() {
		let dragZoom = new DragZoom(this.$refs.canvas);
		const draw = () => {
			dragZoom.draw();
			requestAnimationFrame(draw);
		};

		requestAnimationFrame(draw);
		this.dragZoom = dragZoom;
	},
	methods: {
		setEditor(editor) {
			this.editor = unobserve(editor);
			let self = this;
			editor.onpreviewnode = (node, image) => {
				self.node = unobserve(node);
				self.image = unobserve(image);

				self.dragZoom.setImage(image);

				//this.paint();
			};
		},
		resize(width, height) {
			//console.log("resize");
			fitCanvasToContainer(this.$refs.canvas);

			this.dragZoom.onResize(width, height);

			// repaint
			//this.paint();
		},
		paint() {
			if (!this.image) return;

			let canvas = this.$refs.canvas;
			let ctx = canvas.getContext("2d");

			ctx.drawImage(this.image, 0, 0, canvas.width, canvas.height);
		},
		saveTexture() {
			// todo: save image as png
			//console.log(this.hasImage);
			if (!this.hasImage) return;

			dialog.showSaveDialog(
				remote.getCurrentWindow(),
				{
					filters: [
						{
							name: "PNG",
							extensions: ["png"]
						}
					],
					defaultPath: "image"
				},
				path => {
					if (!path) return;

					let img = this.dragZoom.image;
					let canvas = document.createElement("canvas");
					canvas.width = img.width;
					canvas.height = img.height;
					let ctx = canvas.getContext("2d");
					ctx.drawImage(img, 0, 0);

					// Get the DataUrl from the Canvas
					// https://github.com/mattdesl/electron-canvas-to-buffer/blob/master/index.js
					const url = canvas.toDataURL("image/png", 1);
					const nativeImage = electron.nativeImage.createFromDataURL(url);
					const buffer = nativeImage.toPNG();

					fs.writeFile(path, buffer, function(err) {
						//console.log(err);
						if (err) alert("Error saving image: " + err);
					});
				}
			);
		},
		toggleNineTexures() {
			// todo: display nine textures instead of one to show tiling
			if (this.dragZoom.drawMode == DrawMode.Single)
				this.dragZoom.drawMode = DrawMode.Nine;
			else this.dragZoom.drawMode = DrawMode.Single;
		},
		centerTexture() {
			// todo: center texture in canvas
			this.dragZoom.centerImage();
		},
		reset() {
			this.dragZoom.centerImage();
			this.dragZoom.setImage(null);
		}
	},

	computed: {
		isTiling() {
			return this.dragZoom && this.dragZoom.drawMode == DrawMode.Nine;
		},
		hasImage() {
			return this.dragZoom && this.dragZoom.image != null;
		}
	}
};

//https://stackoverflow.com/questions/10214873/make-canvas-as-wide-and-as-high-as-parent
function fitCanvasToContainer(canvas) {
	// Make it visually fill the positioned parent
	canvas.style.width = "100%";
	// 1em is the size of the top bar
	canvas.style.height = "calc(100% - 2em)";
	// ...then set the internal size to match
	canvas.width = canvas.offsetWidth;
	canvas.height = canvas.offsetHeight;
	//canvas.height = canvas.offsetWidth;

	canvas.style.width = "auto";
	canvas.style.height = "auto";
}
</script>

<style scoped>
.btn {
	text-align: center;
	height: 1.6em;
	/* width: 1.6em; */
	border-radius: 2px;
	line-height: 1.6em;
	display: block;
	float: left;
	padding: 0.4em 0.4em 0.3em 0.4em;
	margin: 0.1em;
	text-decoration: none;
	background: #666;
	color: rgba(255, 255, 255, 0.7);
}

.btn:hover {
	background: #999;
}

.toggled {
	background: #444;
}
</style>
