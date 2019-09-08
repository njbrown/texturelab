<template>
  <div style="height:100%">
    <div style="height:2em;">
      <a class="btn" href="#" @click="saveTexture()">S</a>
      <a class="btn" href="#" @click="toggleNineTexures()">9</a>
    </div>
    <canvas id="_2dpreview" ref="canvas" style="display:block;"></canvas>
  </div>
</template>

<script>
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
      canvas: null
    };
  },
  mounted() {},
  methods: {
    setEditor(editor) {
      this.editor = editor;
      let self = this;
      editor.onpreviewnode = (node, image) => {
        self.node = node;
        self.image = image;

        this.paint();
      };
    },
    resize(width, height) {
      console.log("resize");
      fitCanvasToContainer(this.$refs.canvas);

      // repaint
      this.paint();
    },
    paint() {
      if (!this.image) return;

      let canvas = this.$refs.canvas;
      let ctx = canvas.getContext("2d");

      ctx.drawImage(this.image, 0, 0, canvas.width, canvas.height);
    }
  },
  saveTexture() {
    // todo: save image as png
  },
  toggleNineTexures() {
    // todo: display nine textures instead of one to show tiling
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