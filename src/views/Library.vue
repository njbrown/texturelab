<template>
  <div id="library-view" class="ui container" style="padding:1em;">
    <div class style="padding-bottom:1em; display:flex;">
      <div class="search-container">
        <input type="text" placeholder="Filter Nodes.." v-model="filter" />
      </div>
      <div class="size-container">
        <select>
          <option>Small Icons</option>
          <option>Medium Icons</option>
          <option>Large Icons</option>
          <option>Exra Large Icons</option>
        </select>
      </div>
    </div>
    <div class>
      <span
        v-for="item in filteredList"
        v-on:click="addNode(item.name)"
        v-on:dragstart="dragStart($event, item.name)"
        :key="item.name"
        class="libcard"
        href="#"
        draggable="true"
      >
        <div class="thumbnail">
          <!-- <img v-bind:src="calcImagePath(item.name)" class="ui fluid image" /> -->
          <!-- <span class="thumbnail"></span> -->
        </div>
        <div class="node-name">{{ item.displayName }}</div>
      </span>
    </div>
  </div>
</template>

<script lang="ts">
import { Component, Prop, Model, Vue } from "vue-property-decorator";
import { Editor } from "@/lib/editortest";
import { DesignerLibrary } from "@/lib/designer/library";

@Component
export default class LibraryView extends Vue {
  @Prop()
  library!: DesignerLibrary;

  @Prop()
  editor!: Editor;

  filter: string = "";

  created() {}

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
  display: block;
  float: left;
  margin: 5px;
  cursor: pointer;
}

.thumbnail {
  width: 100px;
  height: 100px;
  background: #ccc;
}

.node-name {
  height: 2.6em;
  line-height: 1.3em;
  padding: 0;
  margin: 0;

  color: white;
  text-decoration: none;
}

.search-container {
  flex-grow: 1;
}

.search-container input {
  width: calc(100% - 2em);
  height: 1.5em;
  padding: 4px;
  margin: 0;
  border-radius: 3px;
  border: solid black 1px;
  color: white;
  background: #999;
}

.size-container {
  flex-grow: 0;
  width: 100px;
}

.size-container select {
  width: 100%;
  height: 100%;
  border: solid white 1px;
  border-radius: 2px;
  color: white;
  background: #222;
  padding: 4px;
}
</style>