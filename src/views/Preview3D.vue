<template>
	<div style="height:100%">
		<div style="padding:5px">
			<!-- <a class="btn" href="#" @click="setShape('sphere')">S</a>
      <a class="btn" href="#" @click="setShape('cube')">C</a>
      <a class="btn" href="#" @click="setShape('plane')">P</a>
      <a class="btn" href="#" @click="setShape('cylinder')">C</a>-->
			<select class="enum" @change="setShape">
				<option value="sphere">Sphere</option>
				<option value="cube">Cube</option>
				<option value="plane">Plane</option>
				<option value="cylinder">Cylinder</option>
				<option value="load">Load..</option>
			</select>

			<button class="enum right" @click="toggleMenu">
				O
			</button>
			<select class="enum right" @change="setTiling">
				<option value="1" selected>Tiling: 1x</option>
				<option value="2">Tiling: 2x</option>
				<option value="3">Tiling: 3x</option>
				<option value="4">Tiling: 4x</option>
			</select>
		</div>
		<div style="display:flex; flex-direction:row;height:calc(100% - 2.2em);">
			<canvas id="_3dpreview" ref="canvas" style="display:block;"></canvas>
			<div v-if="showMenu" class="options-menu">
				<div style="margin-bottom:0.3em;">
					Model:
				</div>
				<div>
					<select class="outlined-enum" @change="setShape">
						<option value="sphere">Sphere</option>
						<option value="cube">Cube</option>
						<option value="plane">Plane</option>
						<option value="cylinder">Cylinder</option>
						<option value="load">Load..</option>
					</select>
				</div>
				<div style="margin-bottom:0.3em;">
					Texture Tiling:
				</div>
				<div>
					<select class="outlined-enum" @change="setTiling">
						<option value="1" selected>Tiling: 1x</option>
						<option value="2">Tiling: 2x</option>
						<option value="3">Tiling: 3x</option>
						<option value="4">Tiling: 4x</option>
					</select>
				</div>
				<div style="margin-bottom:0.3em;">
					Environments:
				</div>
				<div>
					<div v-for="(sky, i) in skies" :key="i" class="sky-card">
						<div>{{ sky.name }}</div>
						<img
							class=""
							style="display:block"
							:src="calcImagePath(sky.thumbnail)"
						/>
					</div>
				</div>
			</div>
		</div>
	</div>
</template>

<script>
import { View3D } from "@/lib/view3d";
import { DesignerNode } from "@/lib/designer/designernode";
import { unobserve } from "../unobserve";
const electron = require("electron");
const remote = electron.remote;
import path from "path";
const { dialog, app, BrowserWindow, Menu } = require("electron").remote;

export default {
	// props: {
	//   editor: {
	//     type: Object
	//   }
	// },
	data() {
		return {
			view3d: null,
			showMenu: false,
			skies: [
				{
					id: "wide_street",
					name: "Wide Street",
					thumbnail: "assets/env/wide_street_thumbnail.png",
					path: "assets/env/wide_street_01_1k.hdr"
				},
				{
					id: "christmas_studio",
					name: "Christmas Studio",
					thumbnail: "assets/env/christmas/christmas_studio_thumbnail.png",
					path: "assets/env/christmas/christmas_photo_studio_01_1k.hdr"
				}
			]
		};
	},
	mounted() {
		this.view3d = unobserve(new View3D());
		this.view3d.setCanvas(this.$refs.canvas);
	},
	methods: {
		setEditor(editor) {
			this.editor = unobserve(editor);
			let self = this;
			// editor.onpreviewnode = (node, image) => {
			//   self.node = node;
			//   self.image = image;

			//   this.paint();
			// };

			editor.ontexturechannelcleared = (imageCanvas, channelName) => {
				this.view3d.clearTexture(channelName);
			};

			editor.ontexturechannelassigned = (imageCanvas, channelName) => {
				this.view3d.setTexture(imageCanvas, channelName);
			};

			editor.ontexturechannelupdated = (imageCanvas, channelName) => {
				this.view3d.updateTexture(channelName);
			};
		},
		resize(width, height) {
			fitCanvasToContainer(this.$refs.canvas, this.showMenu);

			// repaint
			if (this.view3d)
				this.view3d.resize(this.$refs.canvas.width, this.$refs.canvas.height);
		},
		setShape(evt) {
			// todo: set 3d model
			// console.log("set model: ", evt.target.value);
			if (evt.target.value !== "load") this.view3d.setModel(evt.target.value);
			else {
				dialog.showOpenDialog(
					remote.getCurrentWindow(),
					{
						filters: [
							{
								name: "Obj Model",
								extensions: ["obj"]
							}
						],
						defaultPath: ""
					},
					(paths, bookmarks) => {
						let path = paths[0];
						this.view3d.loadModel(path);
					}
				);
			}
		},
		setTiling(evt) {
			// todo: set 3d model
			this.view3d.setRepeat(parseInt(evt.target.value));
		},
		toggleMenu() {
			console.log("toggle menu");
			this.showMenu = !this.showMenu;

			// recalc after render
			this.$nextTick(() => {
				fitCanvasToContainer(this.$refs.canvas, this.showMenu);

				if (this.view3d)
					this.view3d.resize(this.$refs.canvas.width, this.$refs.canvas.height);
			});
		},
		reset() {
			// clear all textures
			// reset camera position
			this.view3d.reset();
		},
		calcImagePath(image) {
			//return `./assets/nodes/${node}.png`;
			if (process.env.NODE_ENV == "production")
				return "file://" + path.join(process.env.BASE_URL, image);
			return path.join(process.env.BASE_URL, image);
		}
	}
};

//https://stackoverflow.com/questions/10214873/make-canvas-as-wide-and-as-high-as-parent
function fitCanvasToContainer(canvas, showMenu) {
	// Make it visually fill the positioned parent
	if (showMenu) canvas.style.width = "50%";
	else canvas.style.width = "100%";

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
	width: 1.6em;
	line-height: 1.6em;
	display: block;
	float: left;
	padding: 0.1em;
	margin: 0.1em;
	text-decoration: none;
	background: #666;
	color: rgba(255, 255, 255, 0.7);
}

.btn:hover {
	background: #999;
}

.enum {
	outline: 0;
	box-shadow: none;
	border: 0 !important;

	border: none;
	border-radius: 4px;
	color: white;
	background: #222;
	padding: 0.5em;
	font-family: "Open Sans";
}

.outlined-enum {
	outline: 0;
	box-shadow: none;
	border: 0 !important;

	border-radius: 4px;
	color: white;
	background: #404040;
	padding: 0.5em;
	font-family: "Open Sans";

	width: 100%;
}

.right {
	float: right;
}

.options-menu {
	color: white;
	display: block;
	width: 50%;
	background: #212121;
	padding: 0.5rem;
	overflow-y: scroll;
}

.sky-card {
	cursor: pointer;
	color: white;

	padding: 0.4em;
	background: #505050;
	border-radius: 3px;

	margin-bottom: 0.3em;
}

.sky-card:hover {
	background: #626262;
}

.sky-card img {
	border-radius: 3px;
	width: 100%;
}

.sky-selected {
	border: solid #a2e4ff 2px;
}
</style>
