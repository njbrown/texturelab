<template>
  <div class="library-container">
    <div class style="padding-bottom:1em; display:flex;margin:0.5em;">
      <div class="search-container">
        <input type="text" placeholder="Filter.." v-model="filter" />
      </div>
      <!-- <div class="size-container">
        <select>
          <option>Small Icons</option>
          <option>Medium Icons</option>
          <option>Large Icons</option>
          <option>Exra Large Icons</option>
        </select>
      </div>-->
    </div>
    <div class="node-list">
      <div style="overflow:hidden;">
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
  padding: 8px;
  cursor: pointer;
  border-radius: 4px;
}

.libcard:hover {
  background: rgb(0, 0, 0, 0.3);
}

.thumbnail {
  width: 100px;
  height: 100px;
  background: #ccc;
  border-radius: 4px;
}

.node-name {
  height: 2.6em;
  line-height: 1.3em;
  padding: 0;
  margin: 0;

  color: white;
  text-decoration: none;
}

.node-list {
  overflow-y: scroll;
  flex: 1 1 auto;
}

.search-container {
  flex-grow: 1;
}

.search-container input {
  /* width: calc(100% - 2em); */
  width: calc(100% - 1em);
  padding: 1em;
  height: 1.5em;
  flex: 0 1 auto;
  padding: 4px;
  margin: 0;
  border-radius: 3px;
  /* border: solid #999 1px; */
  border: 0;
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

.library-container {
  overflow: hidden;
  height: 100%;
  box-sizing: border-box;
  display: flex;
  flex-flow: column;
}
</style>