<template>
  <div style="height:100%">
    <div style="height:2em;">
      <!-- <a class="btn" href="#" @click="setShape('sphere')">S</a>
      <a class="btn" href="#" @click="setShape('cube')">C</a>
      <a class="btn" href="#" @click="setShape('plane')">P</a>
      <a class="btn" href="#" @click="setShape('cylinder')">C</a>-->
      <select class="enum" @change="setShape">
        <option value="sphere">Sphere</option>
        <option value="cube">Cube</option>
        <option value="plane">Plane</option>
        <option value="cylinder">Cylinder</option>
      </select>

      <select class="enum right" @change="setTiling">
        <option value="1">Tiling: 1x</option>
        <option value="2">Tiling: 2x</option>
        <option value="3">Tiling: 3x</option>
        <option value="4" selected>Tiling: 4x</option>
      </select>
    </div>
    <canvas id="_3dpreview" ref="canvas" style="display:block;"></canvas>
  </div>
</template>

<script>
import { View3D } from "@/lib/view3d";
import { DesignerNode } from "@/lib/designer/designernode";

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
      fitCanvasToContainer(this.$refs.canvas);

      // repaint
      if (this.view3d)
        this.view3d.resize(this.$refs.canvas.width, this.$refs.canvas.height);
    },
    setShape(evt) {
      // todo: set 3d model
      console.log("set model: ", evt.target.value);
      this.view3d.setModel(evt.target.value);
    },
    setTiling(evt) {
      // todo: set 3d model
      console.log("set repeat: ", evt.target.value);
      this.view3d.setRepeat(parseInt(evt.target.value));
    },
    reset() {
      // clear all textures
      // reset camera position
      this.view3d.reset();
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
  margin-top: 0.4em;
  border: solid white 1px;
  border-radius: 2px;
  color: white;
  background: #222;
  padding: 4px;
}

.right {
  float: right;
}
</style>