<template>
  <div style="height:100%">
    <div style="height:2em;">
      <a class="btn" href="#" @click="setShape('sphere')">S</a>
      <a class="btn" href="#" @click="setShape('cube')">C</a>
      <a class="btn" href="#" @click="setShape('plane')">P</a>
      <a class="btn" href="#" @click="setShape('cylinder')">C</a>
    </div>
    <canvas id="_3dpreview" ref="canvas" style="display:block;"></canvas>
  </div>
</template>

<script>
import { View3D } from "@/lib/view3d";

export default {
  // props: {
  //   editor: {
  //     type: Object
  //   }
  // },
  data() {
    return {
      view3d: null
    };
  },
  mounted() {
    this.view3d = new View3D();
    this.view3d.setCanvas(this.$refs.canvas);
  },
  methods: {
    setEditor(editor) {
      this.editor = editor;
      let self = this;
      // editor.onpreviewnode = (node, image) => {
      //   self.node = node;
      //   self.image = image;

      //   this.paint();
      // };
    },
    resize(width, height) {
      fitCanvasToContainer(this.$refs.canvas);

      // repaint
      if (this.view3d)
        this.view3d.resize(this.$refs.canvas.width, this.$refs.canvas.height);
    }
  },
  setShape(name) {
    // todo: set 3d model
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
</style>