<template>
  <div id="library-view" class="ui container" style="padding:1em;">
    <div class="ui fluid input" style="padding-bottom:1em;">
      <input type="text" placeholder="filter.." v-model="filter" />
    </div>
    <div class>
      <a
        v-for="item in filteredList"
        v-on:click="addNode(item.name)"
        v-on:dragstart="dragStart($event, item.name)"
        :key="item.name"
        class="libcard"
        href="#"
        draggable="true"
      >
        <div>
          <img v-bind:src="calcImagePath(item.name)" class="ui fluid image" />
        </div>
        <div class="nodecardcontent content">{{item.displayName}}</div>
      </a>
    </div>
  </div>
</template>

<script lang="ts">
import { Component, Prop, Model, Vue } from "vue-property-decorator";
import { DesignerLibrary, DesignerNode } from "@/lib/nodetest";
import { Editor } from "@/lib/editortest";

@Component
export default class LibraryView extends Vue {
  @Prop()
  library!: DesignerLibrary;

  @Prop()
  editor!: Editor;

  @Model()
  filter!: string;

  created() {
    this.filter = "";
  }

  get items() {
    return this.library.nodes;
  }

  get filteredList() {
    var kw = this.filter;
    var list = Object.values(this.items).filter(function(item) {
      return item.name.toLowerCase().includes(kw.toLowerCase());
    });
    return list;
  }

  addNode(nodeName: string) {
    var dnode = this.library.create(nodeName);
    var n = this.editor.addNode(dnode);
    n.setCenter(200, 200);

    return false;
  }

  dragStart(evt: any, name: any) {
    evt.dataTransfer.setData("text/plain", name);
  }

  calcImagePath(node: string) {
    return "/images/nodeicons/" + name + ".png";
  }

  mounted() {}
}
</script>


<style scoped>
.libcard {
  width: 100px;
  height: 100px;
  display: block;
  float: left;
}
</style>