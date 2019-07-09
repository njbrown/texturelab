<template>
  <div>
    <golden-layout class="hscreen">
      <gl-row>
        <gl-col>
          <gl-component title="compA" class="test-component">
            <Editor />
          </gl-component>
        </gl-col>
        <gl-col>
          <gl-component title="compA" class="test-component">
            <h1>CompA</h1>
          </gl-component>
        </gl-col>
        <gl-col>
          <gl-component title="compA" class="test-component">
            <h1>CompA</h1>
          </gl-component>
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
</style>

<script lang="ts">
// https://www.sitepoint.com/class-based-vue-js-typescript/
import { Component, Prop, Vue } from "vue-property-decorator";
import Editor from "@/views/Editor.vue";

@Component({
  components: { Editor }
})
export default class App extends Vue {}
</script>