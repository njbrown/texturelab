<template>
  <!-- <div class="topbar">
      <a class="button" href="#" @click="saveProject()">Save</a>
      <a class="button" href="#">Save As</a>
      <a class="button" href="#" @click="loadProject()">Load</a>

      <a class="right button" href="#">Unity Export</a>
      <a class="right button" href="#">Zip Export</a>
  </div>-->
  <golden-layout class="container" @itemCreated="itemCreated" :headerHeight="30" ref="GL">
    <gl-row>
      <gl-col width="25">
        <gl-component title="2D View" class="test-component" :closable="false">
          <!-- <canvas width="100" height="100" id="_2dview" /> -->
          <preview2d ref="preview2d" />
        </gl-component>

        <gl-component title="3D View" class="test-component" :closable="false">
          <!-- <canvas width="100" height="100" id="_3dview" /> -->
          <preview3d ref="preview3d" />
        </gl-component>
      </gl-col>

      <gl-col width="55" ref="canvas">
        <gl-component title="Editor" class="test-component" :closable="false">
          <div class="editor-menu" style="height:2em;">
            <!-- <a class="btn" href="#" @click="setShape('sphere')">S</a>
      <a class="btn" href="#" @click="setShape('cube')">C</a>
      <a class="btn" href="#" @click="setShape('plane')">P</a>
            <a class="btn" href="#" @click="setShape('cylinder')">C</a>-->
            <select class="enum" :value="resolution" @change="setResolution">
              <option value="256">Resolution: 256x256</option>
              <option value="512">Resolution: 512x512</option>
              <option value="1024">Resolution: 1024x1024</option>
              <option value="2048">Resolution: 2048x2048</option>
              <option value="4096">Resolution: 4096x4096</option>
            </select>
            <span>RandomSeed:</span>
            <input type="number" :value="randomSeed" @change="setRandomSeed" />
          </div>
          <canvas width="400" height="400" id="editor" ondragover="event.preventDefault()" />
        </gl-component>
        <!-- <gl-component title="Library" height="30" :closable="false">
            <library-view :editor="this.editor" :library="this.library" />
        </gl-component>-->
      </gl-col>

      <gl-col width="20">
        <gl-component title="Properties" :closable="false">
          <node-properties-view
            v-if="this.selectedNode != null"
            :editor="this.editor"
            :node="this.selectedNode"
          />
        </gl-component>
        <gl-component title="Library" :closable="false">
          <library-view :editor="this.editor" :library="this.library" v-if="this.library != null" />
        </gl-component>
        <!-- <gl-component title="Texture Properties" class="test-component" :closable="false"></gl-component> -->
      </gl-col>
    </gl-row>
  </golden-layout>
</template>

<style>
body {
  overflow: hidden; /* The 'light' theme let a scroll-bar on the right of the main container */
  padding: 0;
  margin: 0;
}

.lm_tab {
  font-family: "Segoe UI", Tahoma, Geneva, Verdana, sans-serif !important;
  background: #333 !important;
  border-radius: 2px 2px 0 0;
  height: 24px !important;
  box-sizing: border-box;
  line-height: 24px;
}
</style>
<style scoped>
.topbar {
  background: #333;
  border-bottom: 2px black solid;
  flex-grow: 0;
  /* flex-basis: 100px; */
  padding: 0.5em;
  overflow: hidden;
}

.button {
  border-radius: 2px;
  background: #666;
  padding: 0.5em 1em;
  text-decoration: none;
  color: white;
  display: block;
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
  height: calc(100vh - 30px);
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
</style>

<script lang="ts">
// https://www.sitepoint.com/class-based-vue-js-typescript/
import { Component, Prop, Vue, Watch, Model } from "vue-property-decorator";
import EditorView from "@/views/Editor.vue";
import LibraryView from "@/views/Library.vue";
import { Editor } from "@/lib/editortest";
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
const electron = require("electron");
const remote = electron.remote;
const { dialog, app, BrowserWindow, Menu } = remote;

@Component({
  components: {
    EditorView,
    LibraryView,
    NodePropertiesView,
    preview2d: Preview2D,
    preview3d: Preview3D
  }
})
export default class App extends Vue {
  editor!: Editor;
  library!: DesignerLibrary;
  view3d!: View3D;

  selectedNode: DesignerNode = null;

  //designer!: Designer;

  project: Project;

  isMenuSetup: boolean = false;

  resolution: number = 1024;
  randomSeed: number = 32;

  constructor() {
    super();

    this.editor = new Editor();
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

    electron.ipcRenderer.on(MenuCommands.ExportZip, async (evt, arg) => {
      await this.exportZip();
    });
    // electron.ipcRenderer.on(MenuCommands.ExportUnity, async (evt, arg) => {
    //   await this.exportUnity();
    // });
    electron.ipcRenderer.on(MenuCommands.ExportUnityZip, async (evt, arg) => {
      await this.exportUnityZip();
    });

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
    const canv = <HTMLCanvasElement>document.getElementById("editor");
    canv.ondrop = evt => {
      evt.preventDefault();

      var nodeName = evt.dataTransfer.getData("text/plain");
      var dnode = this.library.create(nodeName);
      var rect = canv.getBoundingClientRect();
      var n = this.editor.addNode(
        dnode,
        evt.clientX - rect.left,
        evt.clientY - rect.top
      );
      console.log("drop");
    };
    this.editor.setSceneCanvas(canv);

    //this.designer = this.editor.designer;
    this.editor.onnodeselected = node => {
      this.selectedNode = node;
    };

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

  setupMenu() {
    if (this.isMenuSetup) return;

    // let titleBar = new Titlebar({
    //   backgroundColor: Color.fromHex("#333333"),
    //   icon: "./favicon.svg",
    //   shadow: true
    // });

    this.isMenuSetup = true;
  }

  itemCreated(item: any) {
    // editor
    if (item.config.title == "Editor") {
      let container = item.container;
      item.container.on("resize", function() {
        const canvas = <HTMLCanvasElement>document.getElementById("editor");
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
    this.library = this.editor.library;

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

          this.project.name = path.replace(/^.*[\\\/]/, "");
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
        remote.getCurrentWindow().setTitle(project.name);
        this.editor.load(project.data);
        this.resolution = 1024;
        this.randomSeed = 32;

        this.project = project;
        this.library = this.editor.library;
      }
    );
  }

  loadSample(name: string) {}

  async exportUnity() {
    let buffer = await unityExport(this.editor, this.project.name);
    console.log(buffer);
    dialog.showSaveDialog(remote.getCurrentWindow(), {}, path => {
      if (!path) return;

      fs.writeFile(path, buffer, function(err) {
        if (err) alert("Error exporting texture: " + err);
      });
    });
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
        defaultPath: "material.zip"
      },
      async path => {
        if (!path) {
          return;
        }

        console.log(path);

        let zip = await unityZipExport(this.editor, this.project.name);

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
        defaultPath: "material.zip"
      },
      async path => {
        if (!path) {
          return;
        }

        console.log(path);

        let zip = await zipExport(this.editor, this.project.name);

        zip.writeZip(path);
        remote.shell.showItemInFolder(path);
      }
    );
  }

  showTutorials() {}

  showAboutDialog() {}

  submitBugs() {}

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
