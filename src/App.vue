<template>
  <div>
    <golden-layout class="hscreen" @itemCreated="itemCreated">
      <gl-row>
        <gl-col width="25">
          <gl-component title="compA" class="test-component" :closable="false"></gl-component>
          <gl-component title="compA" class="test-component" :closable="false"></gl-component>
        </gl-col>

        <gl-col width="55" ref="canvas">
          <gl-component title="Editor" class="test-component" height="70" :closable="false">
            <canvas width="400" height="400" id="editor" />
          </gl-component>
          <gl-component title="compA" class="test-component" height="30" :closable="false"></gl-component>
        </gl-col>

        <gl-col width="20">
          <gl-component title="compA" class="test-component" :closable="false"></gl-component>
          <gl-component title="compA" class="test-component" :closable="false"></gl-component>
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

.test-component {
  overflow: hidden;
}
</style>

<script lang="ts">
// https://www.sitepoint.com/class-based-vue-js-typescript/
import { Component, Prop, Vue } from "vue-property-decorator";
import EditorView from "@/views/Editor.vue";
import { Editor } from "@/lib/editortest";

@Component({
  components: { EditorView }
})
export default class App extends Vue {
  editor!: Editor;

  mounted() {
    this.editor = new Editor();

    const canv = <HTMLCanvasElement>document.getElementById("editor");
    this.editor.setSceneCanvas(canv);
    this.editor.createNewTexture();

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
    if (item.config.title == "Editor") {
      let container = item.container;
      item.container.on("resize", function() {
        const canvas = <HTMLCanvasElement>document.getElementById("editor");
        canvas.width = container.width;
        canvas.height = container.height;
      });
    }
  }

  resizeCanvas() {
    console.log("resize!");
  }
}
</script>