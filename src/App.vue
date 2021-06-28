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
				<gl-col width="30">
					<gl-component
						title="2D View"
						class="test-component"
						:closable="false"
						height="40"
					>
						<!-- <canvas width="100" height="100" id="_2dview" /> -->
						<preview2d ref="preview2d" />
					</gl-component>

					<gl-component
						title="3D View"
						class="test-component"
						:closable="false"
						height="60"
					>
						<!-- <canvas width="100" height="100" id="_3dview" /> -->
						<preview3d ref="preview3d" />
					</gl-component>
				</gl-col>

				<gl-col width="50" ref="canvas">
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
							<span class="no-select">RandomSeed:</span>
							<input
								type="number"
								:value="randomSeed"
								@input="updateRandomSeed"
								@blur="setRandomSeed"
								@focus="captureRandomSeed"
							/>
						</div>
						<canvas
							width="400"
							height="400"
							id="editor"
							ondragover="event.preventDefault()"
							:ondrop="handleFileDropInEditor"
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
					<gl-component
						:title="`Library ${libraryVersionString}`"
						:closable="false"
					>
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

.no-drag {
	-webkit-user-drag: none;
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
import { createLibrary as createV2Library } from "@/lib/library/libraryv2";
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
import { IApp } from "./iapp";
import { SetGlobalRandomSeedAction } from "./lib/actions/setglobalrandomseedaction";
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
export default class App extends Vue implements IApp {
	editor!: Editor;
	library!: DesignerLibrary;
	view3d!: View3D;

	selectedNode: DesignerNode = null;
	propHolder: IPropertyHolder = null;

	@Prop()
	titleBar: Titlebar;

	//designer!: Designer;

	project: Project;

	isMenuSetup: boolean = false;

	resolution: number = 1024;
	randomSeed: number = 32;
	oldRandomSeed: number = 32; // for undo/redo purposes

	mouseX: number = 0;
	mouseY: number = 0;

	version: string = "0.3.0";

	constructor() {
		super();

		this.editor = unobserve(new Editor());
		this.library = createV2Library();

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

		electron.ipcRenderer.on(MenuCommands.StylizedGrass, async (evt, arg) => {
			this.openExample("Grass.texture");
		});

		electron.ipcRenderer.on(MenuCommands.Copper, async (evt, arg) => {
			this.openExample("Copper.texture");
		});

		electron.ipcRenderer.on(MenuCommands.FoilGasket, async (evt, arg) => {
			this.openExample("FoilGasket.texture");
		});

		electron.ipcRenderer.on(MenuCommands.Sand, async (evt, arg) => {
			this.openExample("Sand.texture");
		});

		electron.ipcRenderer.on(MenuCommands.YellowTiles, async (evt, arg) => {
			this.openExample("YellowTiles.texture");
		});

		electron.ipcRenderer.on(MenuCommands.StoneGrass, async (evt, arg) => {
			this.openExample("GrassyRock.texture");
		});

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

			console.log("ondrop");
			console.log(evt.dataTransfer.items);
			console.log(evt.dataTransfer.files);

			if (evt.dataTransfer.files.length > 0) {
				this.handleFileDropInEditor(evt.dataTransfer.files);
			} else {
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

		UndoStack.current = this.editor.undoStack;

		// listen for changes in undo stack
		UndoStack.current.cleanStatusChanged = isClean => {
			if (isClean) this.setWindowTitle(this.project.name);
			else this.setWindowTitle("*" + this.project.name);
		};

		// handle quit event
		window.onbeforeunload = (e: BeforeUnloadEvent) => {
			console.log(e);
			e.returnValue = false;

			// this.$nextTick(() => {
			// 	this.handleSaveBeforeQuit(e);
			// });

			setTimeout(() => {
				this.handleSaveBeforeQuit(e);
			}, 0);

			// this.handleSaveBeforeQuit(e);
			// alert("Should i save?");
			// console.log(e);

			// remote.getCurrentWindow().close();
		};

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
			//console.log(this.$refs.GL);
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

	get libraryVersionString() {
		return this.library == null ? "" : `(${this.library.versionName})`;
	}

	showLibraryMenu() {
		// ensure mouse is in canvas bounds
		//if (this.$refs.canvas.offset)
		let lib = this.$refs.libraryMenu as any;

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

	resizeCanvas() {}

	setWindowTitle(newTitle: string) {
		document.title = newTitle;
		this.titleBar.updateTitle();
	}

	handleSaveBeforeQuit(e: BeforeUnloadEvent) {
		// close browser without check
		const closeWindow = () => {
			window.onbeforeunload = null;
			window.close();
			// this.$nextTick(() => {
			// 	window.onbeforeunload = null;
			// 	window.close();
			// 	//remote.getCurrentWindow().close();
			// });
		};

		if (!UndoStack.current.isClean()) {
			const action = dialog.showMessageBoxSync({
				message: "You have unsaved changes. Do you want to save them?",
				buttons: ["Yes", "No", "Cancel"]
			});

			// canceled
			if (action == 2) {
				e.returnValue = false;
				return;
			}

			if (action == 1) {
				closeWindow();
			}

			// yes, save scene
			if (action == 0) {
				e.returnValue = false;
				this.saveProject(false, () => closeWindow());
			}
		} else {
			closeWindow();
		}
	}

	handleFileDropInEditor(files: FileList) {
		// todo: handle image drop differently
		// for now, only handle file drop
		const file = files[0];
		if (file.path.endsWith(".texture")) {
			const openProject = () => {
				// reset states of all components
				// load default scene
				(this.$refs.preview3d as any).reset();
				(this.$refs.preview2d as any).reset();

				let project = ProjectManager.load(file.path);

				// ensure library exists
				let libName = project.data["libraryVersion"];
				let libraries = ["v0", "v1", "v2"];
				if (libraries.indexOf(libName) == -1) {
					alert(
						`Project contains unknown library version '${libName}'. It must have been created with a new version of TextureLab`
					);
					return;
				}

				this.editor.load(project.data);
				this.resolution = 1024;
				this.randomSeed = 32;

				this.project = unobserve(project);
				this.library = unobserve(this.editor.library);

				this.setWindowTitle(project.name);

				UndoStack.current.clear();
			};

			if (!UndoStack.current.isClean()) {
				const action = dialog.showMessageBoxSync(null, {
					message: "You have unsaved changes. Do you want to save them?",
					buttons: ["Yes", "No", "Cancel"]
				});

				// canceled
				if (action == 2) {
					return;
				}

				if (action == 1) {
					openProject();
				}

				// yes, save scene
				if (action == 0) {
					this.saveProject(false, () => openProject());
				}
			} else {
				openProject();
			}
		}
	}

	newProject() {
		const newProject = () => {
			// reset states of all components
			// load default scene
			(this.$refs.preview3d as any).reset();
			(this.$refs.preview2d as any).reset();

			this.editor.createNewTexture();
			this.library = unobserve(this.editor.library);

			this.project.name = "Untitled Project";
			this.project.path = null;

			this.resolution = 1024;
			this.randomSeed = 32;

			this.setWindowTitle(this.project.name);
			UndoStack.current.clear();
		};

		if (!UndoStack.current.isClean()) {
			const action = dialog.showMessageBoxSync(null, {
				message: "You have unsaved changes. Do you want to save them?",
				buttons: ["Yes", "No", "Cancel"]
			});

			// canceled
			if (action == 2) {
				return;
			}

			if (action == 1) {
				newProject();
			}

			// yes, save scene
			if (action == 0) {
				this.saveProject(false, () => newProject());
			}
		} else {
			newProject();
		}
	}

	saveProject(saveAs: boolean = false, onSuccess: () => void = null) {
		var data = this.editor.save();
		// console.log(data);
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
					if (!path) return;

					if (!path.endsWith(".texture")) path += ".texture";

					this.project.name = path.replace(/^.*[\\/]/, "");
					this.project.path = path;

					ProjectManager.save(path, this.project);
					this.setWindowTitle(this.project.name);
					UndoStack.current.setClean();

					if (onSuccess) onSuccess();
				}
			);
		} else {
			ProjectManager.save(this.project.path, this.project);
			UndoStack.current.setClean();
		}
	}

	openProject() {
		const openProject = () => {
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
					if (!paths) return;

					let path = paths[0];

					let project = ProjectManager.load(path);
					// console.log(project);

					// ensure library exists
					let libName = project.data["libraryVersion"];
					let libraries = ["v0", "v1", "v2"];
					if (libraries.indexOf(libName) == -1) {
						alert(
							`Project contains unknown library version '${libName}'. It must have been created with a new version of TextureLab`
						);
						return;
					}

					this.editor.load(project.data);
					this.resolution = 1024;
					this.randomSeed = 32;

					this.project = unobserve(project);
					this.library = unobserve(this.editor.library);

					this.setWindowTitle(project.name);

					UndoStack.current.clear();
				}
			);
		};

		if (!UndoStack.current.isClean()) {
			const action = dialog.showMessageBoxSync(null, {
				message: "You have unsaved changes. Do you want to save them?",
				buttons: ["Yes", "No", "Cancel"]
			});

			// canceled
			if (action == 2) {
				return;
			}

			if (action == 1) {
				openProject();
			}

			// yes, save scene
			if (action == 0) {
				this.saveProject(false, () => openProject());
			}
		} else {
			openProject();
		}
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
		// console.log(project);

		// ensure library exists
		let libName = project.data["libraryVersion"];
		let libraries = ["v0", "v1", "v2"];
		if (libraries.indexOf(libName) == -1) {
			alert(
				`Project contains unknown library version '${libName}'. It must have been created with a new version of TextureLab`
			);
			return;
		}

		const openProject = () => {
			this.editor.load(project.data);
			this.resolution = 1024;
			this.randomSeed = 32;

			project.path = null; // this ensures saving pops SaveAs dialog
			this.project = unobserve(project);
			this.library = unobserve(this.editor.library);
			this.setWindowTitle(project.name);

			UndoStack.current.clear();
		};

		if (!UndoStack.current.isClean()) {
			const action = dialog.showMessageBoxSync(null, {
				message: "You have unsaved changes. Do you want to save them?",
				buttons: ["Yes", "No", "Cancel"]
			});

			// canceled
			if (action == 2) {
				return;
			}

			if (action == 1) {
				openProject();
			}

			// yes, save scene
			if (action == 0) {
				this.saveProject(false, () => openProject());
			}
		} else {
			openProject();
		}
	}

	setResolution(evt) {
		let value = parseInt(evt.target.value);
		//console.log(value);
		this.resolution = value;
		this.editor.designer.setTextureSize(value, value);
	}

	// called when the input is focused
	captureRandomSeed(evt) {
		this.oldRandomSeed = this.randomSeed;
	}

	// called on input
	updateRandomSeed(evt) {
		let seed = evt.target.value;
		this.randomSeed = seed;
		this.editor.designer.setRandomSeed(seed);
	}

	// called when the input is blurred
	setRandomSeed(evt) {
		if (this.randomSeed == this.oldRandomSeed) return;

		UndoStack.current.push(
			new SetGlobalRandomSeedAction(
				this,
				this.editor,
				this.oldRandomSeed,
				this.randomSeed
			)
		);

		// let seed = evt.target.value;
		// this.randomSeed = seed;
		this.editor.designer.setRandomSeed(this.randomSeed);
	}
}
</script>
