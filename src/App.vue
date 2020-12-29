<template>
	<div>
		<div class="topbar no-select">
			<a class="button" href="#" @click="undoAction()">
				<i class="bx bx-undo" style="font-size:1.4rem !important;"></i>Undo
			</a>
			<a class="button" href="#" @click="redoAction()">
				<i class="bx bx-redo" style="font-size:1.4rem !important;"></i>Redo
			</a>

			<a class="right button" href="#" @click="exportUnity()">Unity Export</a>
			<a class="right button" href="#" @click="exportZip()">Zip Export</a>
		</div>
		<golden-layout
			class="container"
			@itemCreated="itemCreated"
			:headerHeight="30"
			:showPopoutIcon="false"
			:showMaximiseIcon="false"
			ref="GL"
		>
			<gl-row>
				<gl-col width="25">
					<gl-component
						title="2D View"
						class="test-component"
						:closable="false"
					>
						<!-- <canvas width="100" height="100" id="_2dview" /> -->
						<preview2d ref="preview2d" />
					</gl-component>

					<gl-component
						title="3D View"
						class="test-component"
						:closable="false"
					>
						<!-- <canvas width="100" height="100" id="_3dview" /> -->
						<preview3d ref="preview3d" />
					</gl-component>
				</gl-col>

				<gl-col width="55" ref="canvas">
					<gl-component title="Editor" class="test-component" :closable="false">
						<library-menu
							:editor="this.editor"
							:library="this.library"
							v-if="this.library != null"
							ref="libraryMenu"
						/>
						<div class="editor-menu" style="height:2em;">
							<!-- <a class="btn" href="#" @click="setShape('sphere')">S</a>
      <a class="btn" href="#" @click="setShape('cube')">C</a>
      <a class="btn" href="#" @click="setShape('plane')">P</a>
							<a class="btn" href="#" @click="setShape('cylinder')">C</a>-->
							<select class="enum" :value="resolution" @change="setResolution">
								<option value="32">Resolution: 32x32</option>
								<option value="64">Resolution: 64x64</option>
								<option value="128">Resolution: 128x128</option>
								<option value="256">Resolution: 256x256</option>
								<option value="512">Resolution: 512x512</option>
								<option value="1024">Resolution: 1024x1024</option>
								<option value="2048">Resolution: 2048x2048</option>
								<option value="4096">Resolution: 4096x4096</option>
							</select>
							<span>RandomSeed:</span>
							<input
								type="number"
								:value="randomSeed"
								@change="setRandomSeed"
							/>
						</div>
						<canvas
							width="400"
							height="400"
							id="editor"
							ondragover="event.preventDefault()"
						/>
					</gl-component>
					<!-- <gl-component title="Library" height="30" :closable="false">
            <library-view :editor="this.editor" :library="this.library" />
					</gl-component>-->
				</gl-col>

				<gl-col width="20">
					<gl-component title="Properties" :closable="false">
						<node-properties-view
							ref="properties"
							v-if="this.propHolder != null"
							:editor="this.editor"
							:node="this.propHolder"
						/>
					</gl-component>
					<gl-component title="Library" :closable="false">
						<library-view
							:editor="this.editor"
							:library="this.library"
							v-if="this.library != null"
						/>
					</gl-component>
					<!-- <gl-component title="Texture Properties" class="test-component" :closable="false"></gl-component> -->
				</gl-col>
			</gl-row>
		</golden-layout>
	</div>
</template>

<style>
body {
	overflow: hidden; /* The 'light' theme let a scroll-bar on the right of the main container */
	padding: 0;
	margin: 0;
}

.lm_tab {
	font-family: "Open Sans", "Segoe UI", Tahoma, Geneva, Verdana, sans-serif !important;
	background: #333 !important;
	/* border-radius: 2px 2px 0 0; */
	height: 24px !important;
	box-sizing: border-box;
	line-height: 24px;
	font-weight: bold;
	-webkit-user-select: none;
	-webkit-user-drag: none;
	-webkit-app-region: no-drag;
}

.no-select {
	-webkit-user-select: none;
	-webkit-user-drag: none;
	-webkit-app-region: no-drag;
}
</style>
<style scoped>
.topbar {
	background: #333;
	border-bottom: 2px black solid;
	flex-grow: 0;
	/* flex-basis: 100px; */
	padding: 0.5em 0;
	overflow: hidden;
	padding-left: 0.5em;
}

.button {
	border-radius: 2px;
	background: #666;
	padding: 0.5em 1em;
	text-decoration: none;
	color: white;
	display: flex;
	vertical-align: middle;
	float: left;
	margin-right: 0.5em;
}

.button:hover {
	background: #999;
}

.button:active {
	background: #555;
}

.right {
	float: right;
}

.hscreen {
	/* width: 100vw; */
	/* height: calc(100vh - 2em); */
	/* height: 100%; */
	padding: 0;
	margin: 0;
	/* flex-grow: 1; */
}

.container {
	display: block;
	width: 100vw;
	height: calc(100vh - 30px - 38px - (2 * 0.5em) - 2px);
	/* flex-direction: column; */
}

.glComponent {
	background: #333;
}
.test-component {
	overflow: hidden;
	background: #333;
}

.editor-menu .enum {
	margin-top: 0.1em;
	border: solid white 1px;
	border-radius: 2px;
	color: white;
	background: #222;
	padding: 4px;
	margin-right: 5px;
}

.editor-menu span {
	color: white;
}

.editor-menu input[type="number"] {
	/* //color: white; */
	margin: 5px;
	border: solid white 1px;
	border-radius: 2px;
	line-height: 1.5em;
	width: 3em;
	padding: 0 5px;
}

.enum {
	outline: 0;
	box-shadow: none;
	border: 0 !important;

	margin-top: 0.4em;
	border: none;
	border-radius: 4px;
	color: white;
	background: #222;
	padding: 0.5em;
	font-family: "Open Sans";
}
</style>

<script lang="ts">
// https://www.sitepoint.com/class-based-vue-js-typescript/
import { Component, Prop, Vue, Watch, Model } from "vue-property-decorator";
import EditorView from "@/views/Editor.vue";
import LibraryView from "@/views/Library.vue";
import LibraryMenu from "@/components/LibraryMenu.vue";
import { Editor } from "@/lib/editor";
import { View3D } from "@/lib/view3d";
import { createLibrary as createV1Library } from "@/lib/library/libraryv1";
import NodePropertiesView from "./views/NodeProperties.vue";
import Preview2D from "./views/Preview2D.vue";
import Preview3D from "./views/Preview3D.vue";
import { DesignerLibrary } from "./lib/designer/library";
import { DesignerNode } from "./lib/designer/designernode";
import { Designer } from "./lib/designer";
import { Project, ProjectManager } from "./lib/project";
import { Titlebar, Color } from "custom-electron-titlebar";
import { MenuCommands } from "./menu";
import { unityExport } from "@/lib/export/unityexporter.js";
import { unityZipExport } from "@/lib/export/unityzipexporter.js";
import { zipExport } from "@/lib/export/zipexporter.js";
import { shell } from "electron";
//import libv1 from "./lib/library/libraryv1";
import fs from "fs";
import path from "path";
import { IPropertyHolder } from "./lib/designer/properties";
import { AddItemsAction } from "./lib/actions/additemsaction";
import { UndoStack } from "./lib/undostack";
import { unobserve } from "./unobserve";
const electron = require("electron");
const remote = electron.remote;
const { dialog, app, BrowserWindow, Menu } = remote;

declare var __static: any;

@Component({
	components: {
		EditorView,
		LibraryView,
		NodePropertiesView,
		LibraryMenu,
		preview2d: Preview2D,
		preview3d: Preview3D
	}
})
export default class App extends Vue {
	editor!: Editor;
	library!: DesignerLibrary;
	view3d!: View3D;

	selectedNode: DesignerNode = null;
	propHolder: IPropertyHolder = null;

	//designer!: Designer;

	project: Project;

	isMenuSetup: boolean = false;

	resolution: number = 1024;
	randomSeed: number = 32;

	mouseX: number = 0;
	mouseY: number = 0;

	version: string = "0.1.0";

	constructor() {
		super();

		this.editor = unobserve(new Editor());
		this.library = null;

		this.project = new Project();
	}

	created() {
		electron.ipcRenderer.on(MenuCommands.FileNew, (evt, arg) => {
			this.newProject();
		});
		electron.ipcRenderer.on(MenuCommands.FileOpen, (evt, arg) => {
			this.openProject();
		});
		electron.ipcRenderer.on(MenuCommands.FileSave, (evt, arg) => {
			this.saveProject();
		});
		electron.ipcRenderer.on(MenuCommands.FileSaveAs, (evt, arg) => {
			this.saveProject(true);
		});

		electron.ipcRenderer.on(MenuCommands.EditUndo, async (evt, arg) => {
			this.undoAction();
		});
		electron.ipcRenderer.on(MenuCommands.EditRedo, async (evt, arg) => {
			this.redoAction();
		});

		electron.ipcRenderer.on(MenuCommands.EditCopy, async (evt, arg) => {
			document.execCommand("copy");
		});
		electron.ipcRenderer.on(MenuCommands.EditCut, async (evt, arg) => {
			document.execCommand("cut");
		});
		electron.ipcRenderer.on(MenuCommands.EditPaste, async (evt, arg) => {
			document.execCommand("paste");
		});

		electron.ipcRenderer.on(MenuCommands.ExportZip, async (evt, arg) => {
			await this.exportZip();
		});
		electron.ipcRenderer.on(MenuCommands.ExportUnity, async (evt, arg) => {
			await this.exportUnity();
		});
		electron.ipcRenderer.on(MenuCommands.ExportUnityZip, async (evt, arg) => {
			await this.exportUnityZip();
		});

		// samples
		electron.ipcRenderer.on(
			MenuCommands.ExamplesGoldLinesMarbleTiles,
			async (evt, arg) => {
				this.openExample("GoldLinedMarbleTiles.texture");
			}
		);

		electron.ipcRenderer.on(MenuCommands.ExamplesGrenade, async (evt, arg) => {
			this.openExample("Grenade.texture");
		});

		electron.ipcRenderer.on(MenuCommands.ExamplesScrews, async (evt, arg) => {
			this.openExample("Screws.texture");
		});

		electron.ipcRenderer.on(
			MenuCommands.ExamplesWoodenPlanks,
			async (evt, arg) => {
				this.openExample("WoodenPlanks.texture");
			}
		);

		electron.ipcRenderer.on(MenuCommands.HelpTutorials, (evt, arg) => {
			this.showTutorials();
		});
		electron.ipcRenderer.on(MenuCommands.HelpAbout, (evt, arg) => {
			this.showAboutDialog();
		});
		electron.ipcRenderer.on(MenuCommands.HelpSubmitBug, (evt, arg) => {
			this.submitBugs();
		});
	}

	mounted() {
		this.setupMenu();

		document.addEventListener("mousemove", evt => {
			this.mouseX = evt.pageX;
			this.mouseY = evt.pageY;
		});

		const canv = document.getElementById("editor") as HTMLCanvasElement;
		canv.ondrop = evt => {
			evt.preventDefault();

			var itemJson = evt.dataTransfer.getData("text/plain");
			let item = JSON.parse(itemJson);
			let rect = canv.getBoundingClientRect();
			var pos = this.editor.graph.view.canvasToSceneXY(
				evt.clientX - rect.left,
				evt.clientY - rect.top
			);

			let action: AddItemsAction = null;

			if (item.type == "node") {
				let nodeName = item.name;

				let dnode = this.library.create(nodeName);
				// let n = this.editor.addNode(
				// 	dnode,
				// 	evt.clientX - rect.left,
				// 	evt.clientY - rect.top
				// );
				let n = this.editor.addNode(dnode);
				n.setCenter(pos.x, pos.y);

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
			} else if (item.type == "comment") {
				let d = this.editor.createComment();
				d.setCenter(pos.x, pos.y);

				action = new AddItemsAction(
					this.editor.graph,
					this.editor.designer,
					[],
					[d],
					[],
					[],
					[],
					[]
				);
			} else if (item.type == "frame") {
				let d = this.editor.createFrame();
				d.setCenter(pos.x, pos.y);

				action = new AddItemsAction(
					this.editor.graph,
					this.editor.designer,
					[d],
					[],
					[],
					[],
					[],
					[]
				);
			} else if (item.type == "navigation") {
				let d = this.editor.createNavigation();
				d.setCenter(pos.x, pos.y);

				action = new AddItemsAction(
					this.editor.graph,
					this.editor.designer,
					[],
					[],
					[d],
					[],
					[],
					[]
				);
			}

			if (action != null) {
				UndoStack.current.push(action);
			}
		};
		this.editor.setSceneCanvas(canv);

		//this.designer = this.editor.designer;
		this.editor.onnodeselected = node => {
			//this.selectedNode = node;
			this.propHolder = unobserve(node);
		};
		this.editor.oncommentselected = comment => {
			this.propHolder = unobserve(comment);
			console.log("comment selected");
		};
		this.editor.onframeselected = frame => {
			this.propHolder = unobserve(frame);
		};
		// this.editor.onnavigationselected = nav => {
		//   this.propHolder = nav;
		// };
		this.editor.onlibrarymenu = this.showLibraryMenu;

		// const _2dview = <HTMLCanvasElement>document.getElementById("_2dview");
		// this.editor.set2DPreview(_2dview);
		(this.$refs.preview2d as any).setEditor(this.editor);

		// const _3dview = <HTMLCanvasElement>document.getElementById("_3dview");
		// this.view3d = new View3D();
		// this.view3d.setCanvas(_3dview);
		//this.editor.set3DScene(scene3D);
		(this.$refs.preview3d as any).setEditor(this.editor);

		//this.editor.createNewTexture();
		this.newProject();

		// start animation
		const draw = () => {
			if (this.editor) {
				// might not be loaded as yet
				this.editor.update();
				this.editor.draw();
			}
			requestAnimationFrame(draw);
		};
		requestAnimationFrame(draw);

		// properly resize GL
		window.addEventListener("resize", () => {
			console.log(this.$refs.GL);
		});
	}

	undoAction() {
		if (document.activeElement instanceof HTMLElement)
			(document.activeElement as HTMLElement).blur();
		this.editor.undo();
	}

	redoAction() {
		if (document.activeElement instanceof HTMLElement)
			(document.activeElement as HTMLElement).blur();
		this.editor.redo();
	}

	setupMenu() {
		if (this.isMenuSetup) return;

		// let titleBar = new Titlebar({
		//   backgroundColor: Color.fromHex("#333333"),
		//   icon: "./favicon.svg",
		//   shadow: true
		// });

		this.isMenuSetup = true;
	}

	showLibraryMenu() {
		// ensure mouse is in canvas bounds
		//if (this.$refs.canvas.offset)
		let lib = this.$refs.libraryMenu as any;
		console.log("show menu");
		if (lib.show == false) lib.showModal(this.mouseX, this.mouseY);
	}

	itemCreated(item: any) {
		// editor
		if (item.config.title == "Editor") {
			let container = item.container;
			item.container.on("resize", function() {
				const canvas = document.getElementById("editor") as HTMLCanvasElement;
				canvas.width = container.width;
				canvas.height = container.height - 32;
			});
		}

		// 2d view
		if (item.config.title == "2D View") {
			let container = item.container;
			item.container.on("resize", () => {
				// const canvas = <HTMLCanvasElement>document.getElementById("_2dview");
				// canvas.width = container.width;
				// canvas.height = container.height;

				(this.$refs.preview2d as any).resize(container.width, container.height);
			});
		}

		// 3d view
		if (item.config.title == "3D View") {
			let container = item.container;
			item.container.on("resize", () => {
				// const canvas = <HTMLCanvasElement>document.getElementById("_3dview");
				// canvas.width = container.width;
				// canvas.height = container.height;
				//if (this.view3d) this.view3d.resize(container.width, container.height);
				(this.$refs.preview3d as any).resize(container.width, container.height);
			});
		}
	}

	resizeCanvas() {
		console.log("resize!");
	}

	newProject() {
		// reset states of all components
		// load default scene
		(this.$refs.preview3d as any).reset();
		(this.$refs.preview2d as any).reset();

		this.editor.createNewTexture();
		this.library = unobserve(this.editor.library);

		this.project.name = "New Texture";
		this.project.path = null;

		this.resolution = 1024;
		this.randomSeed = 32;

		// todo: set title
	}

	saveProject(saveAs: boolean = false) {
		var data = this.editor.save();
		console.log(data);
		this.project.data = data;
		this.project.data["appVersion"] = this.version;

		// if project has no name then it hasnt been saved yet
		if (this.project.path == null || saveAs) {
			dialog.showSaveDialog(
				remote.getCurrentWindow(),
				{
					filters: [
						{
							name: "TextureLab Texture",
							extensions: ["texture"]
						}
					],
					defaultPath: "material.texture"
				},
				path => {
					//console.log(path);
					if (!path.endsWith(".texture")) path += ".texture";

					this.project.name = path.replace(/^.*[\\/]/, "");
					this.project.path = path;

					ProjectManager.save(path, this.project);
					remote.getCurrentWindow().setTitle(this.project.name);
				}
			);
		} else {
			ProjectManager.save(this.project.path, this.project);
		}
	}

	openProject() {
		// ask if current project should be saved
		dialog.showOpenDialog(
			remote.getCurrentWindow(),
			{
				filters: [
					{
						name: "TextureLab Texture",
						extensions: ["texture"]
					}
				],
				defaultPath: "material"
			},
			(paths, bookmarks) => {
				let path = paths[0];

				let project = ProjectManager.load(path);
				console.log(project);

				// ensure library exists
				let libName = project.data["libraryVersion"];
				let libraries = ["v0", "v1"];
				if (libraries.indexOf(libName) == -1) {
					alert(
						`Project contains unknown library version '${libName}'. It must have been created with a new version of TextureLab`
					);
					return;
				}

				remote.getCurrentWindow().setTitle(project.name);
				this.editor.load(project.data);
				this.resolution = 1024;
				this.randomSeed = 32;

				this.project = unobserve(project);
				this.library = unobserve(this.editor.library);
			}
		);
	}

	loadSample(name: string) {}

	async exportUnity() {
		let buffer = await unityExport(
			this.editor,
			this.project.name.replace(".texture", "")
		);
		//console.log(buffer);
		dialog.showSaveDialog(
			remote.getCurrentWindow(),
			{
				filters: [
					{
						name: "Unity Package",
						extensions: ["unitypackage"]
					}
				],
				defaultPath:
					(this.project.name
						? this.project.name.replace(".texture", "")
						: "material") + ".unitypackage"
			},
			path => {
				if (!path) return;

				fs.writeFile(path, buffer, function(err) {
					if (err) alert("Error exporting texture: " + err);
				});

				remote.shell.showItemInFolder(path);
			}
		);
	}

	exportUnityZip() {
		dialog.showSaveDialog(
			remote.getCurrentWindow(),
			{
				filters: [
					{
						name: "Zip File",
						extensions: ["zip"]
					}
				],
				defaultPath:
					(this.project.name
						? this.project.name.replace(".texture", "")
						: "material") + ".zip"
			},
			async path => {
				if (!path) {
					return;
				}

				console.log(path);

				let zip = await unityZipExport(
					this.editor,
					this.project.name.replace(".texture", "")
				);

				zip.writeZip(path);
				remote.shell.showItemInFolder(path);
			}
		);
	}

	exportZip() {
		dialog.showSaveDialog(
			remote.getCurrentWindow(),
			{
				filters: [
					{
						name: "Zip File",
						extensions: ["zip"]
					}
				],
				defaultPath:
					(this.project.name
						? this.project.name.replace(".texture", "")
						: "material") + ".zip"
			},
			async path => {
				if (!path) {
					return;
				}

				console.log(path);

				let zip = await zipExport(
					this.editor,
					this.project.name.replace(".texture", "")
				);

				zip.writeZip(path);
				remote.shell.showItemInFolder(path);
			}
		);
	}

	showTutorials() {}

	showAboutDialog() {}

	submitBugs() {}

	openExample(fileName: string) {
		let fullPath = path.join(__static, "assets/examples/", fileName);

		this._openSample(fullPath);
	}

	_openSample(path: string) {
		let project = ProjectManager.load(path);
		console.log(project);

		// ensure library exists
		let libName = project.data["libraryVersion"];
		let libraries = ["v0", "v1"];
		if (libraries.indexOf(libName) == -1) {
			alert(
				`Project contains unknown library version '${libName}'. It must have been created with a new version of TextureLab`
			);
			return;
		}

		remote.getCurrentWindow().setTitle(project.name);
		this.editor.load(project.data);
		this.resolution = 1024;
		this.randomSeed = 32;

		project.path = null; // this ensures saving pops SaveAs dialog
		this.project = unobserve(project);
		this.library = unobserve(this.editor.library);
	}

	setResolution(evt) {
		let value = parseInt(evt.target.value);
		//console.log(value);
		this.resolution = value;
		this.editor.designer.setTextureSize(value, value);
	}

	setRandomSeed(evt) {
		let seed = evt.target.value;
		this.randomSeed = seed;
		this.editor.designer.setRandomSeed(seed);
	}
}
</script>
