<template>
  <div>
    <golden-layout class="hscreen" @itemCreated="itemCreated">
      <gl-row>
        <gl-col width="25">
          <gl-component title="2D View" class="test-component" :closable="false">
            <canvas width="100" height="100" id="_2dview" />
          </gl-component>

          <gl-component title="3D View" class="test-component" :closable="false">
            <canvas width="100" height="100" id="_3dview" />
          </gl-component>
        </gl-col>

        <gl-col width="55" ref="canvas">
          <gl-component title="Editor" class="test-component" height="70" :closable="false">
            <canvas width="400" height="400" id="editor" ondragover="event.preventDefault()" />
          </gl-component>
          <gl-component title="Library" height="30" :closable="false">
            <library-view :editor="this.editor" :library="this.library" />
          </gl-component>
        </gl-col>

        <gl-col width="20">
          <gl-component title="Properties" :closable="false">
            <node-properties-view
              v-if="this.selectedNode!=null"
              :editor="this.editor"
              :node="this.selectedNode"
            />
          </gl-component>
          <gl-component title="Texture Properties" class="test-component" :closable="false"></gl-component>
        </gl-col>
      </gl-row>
    </golden-layout>
  </div>
</template>

<style>
.hscreen {
  width: 100vw;
  height: 100vh;
  padding: 0;
  margin: 0;
}
body {
  overflow: hidden; /* The 'light' theme let a scroll-bar on the right of the main container */
  padding: 0;
  margin: 0;
}

.glComponent {
  background: #333;
}
.test-component {
  overflow: hidden;
  background: #333;
}
</style>

<script lang="ts">
// https://www.sitepoint.com/class-based-vue-js-typescript/
import { Component, Prop, Vue, Watch, Model } from "vue-property-decorator";
import EditorView from "@/views/Editor.vue";
import LibraryView from "@/views/Library.vue";
import { Editor } from "@/lib/editortest";
import { View3D } from "@/lib/view3d";
import { createLibrary } from "@/lib/library/libraryv1";
import { DesignerLibrary, DesignerNode, Designer } from "@/lib/nodetest";
import NodePropertiesView from "./views/NodeProperties.vue";

@Component({
  components: { EditorView, LibraryView, NodePropertiesView }
})
export default class App extends Vue {
  editor!: Editor;
  library!: DesignerLibrary;
  view3d!: View3D;

  selectedNode: DesignerNode = null;

  designer!: Designer;

  constructor() {
    super();

    this.editor = new Editor();
    this.library = createLibrary();
  }

  created() {}

  mounted() {
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
    this.editor.createNewTexture();

    this.designer = this.editor.designer;
    this.editor.onnodeselected = node => {
      this.selectedNode = node;
    };

    const _2dview = <HTMLCanvasElement>document.getElementById("_2dview");
    this.editor.set2DPreview(_2dview);

    const _3dview = <HTMLCanvasElement>document.getElementById("_3dview");
    this.view3d = new View3D();
    this.view3d.setCanvas(_3dview);
    //this.editor.set3DScene(scene3D);

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
  }

  itemCreated(item: any) {
    // editor
    if (item.config.title == "Editor") {
      let container = item.container;
      item.container.on("resize", function() {
        const canvas = <HTMLCanvasElement>document.getElementById("editor");
        canvas.width = container.width;
        canvas.height = container.height;
      });
    }

    // 2d view
    if (item.config.title == "2D View") {
      let container = item.container;
      item.container.on("resize", function() {
        const canvas = <HTMLCanvasElement>document.getElementById("_2dview");
        canvas.width = container.width;
        canvas.height = container.height;
      });
    }

    // 3d view
    if (item.config.title == "3D View") {
      let container = item.container;
      item.container.on("resize", () => {
        // const canvas = <HTMLCanvasElement>document.getElementById("_3dview");
        // canvas.width = container.width;
        // canvas.height = container.height;

        this.view3d.resize(container.width, container.height);
      });
    }
  }

  resizeCanvas() {
    console.log("resize!");
  }
}
</script>