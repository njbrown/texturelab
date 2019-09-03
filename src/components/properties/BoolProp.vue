<template>
  <div class="field">
    <label>{{prop.displayName}}</label>
    <div>
      <button class="bool" @click="toggleValue()">{{ valueText }}</button>
    </div>
  </div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit, Model } from "vue-property-decorator";
import { Designer, DesignerNode } from "@/lib/nodetest";

@Component
export default class BoolPropertyView extends Vue {
  @Prop()
  // BoolProperty
  prop: any;

  @Prop()
  designer: Designer;

  @Prop()
  node: DesignerNode;

  @Emit()
  propertyChanged() {
    return this.prop.name;
  }

  value: boolean = true;
  get valueText() {
    return this.value ? "True" : "False";
  }
  mounted() {
    this.value = this.prop.value;
  }

  toggleValue() {
    this.value = !this.value;
    this.node.setProperty(this.prop.name, this.value);
    this.propertyChanged();
  }
}
</script>

<style scoped>
.field {
  font-size: 12px;
  padding: 0.9em 0.5em;
  color: rgba(255, 255, 255, 0.7);
  border-bottom: 1px rgb(61, 61, 61) solid;
}

.field label {
  font-weight: bold;
  padding: 0.4em;
  padding-left: 0;
}

.bool {
  margin-top: 0.4em;
  width: 100%;
  border: solid white 1px;
  border-radius: 2px;
  color: white;
  background: #222;
  padding: 4px;
}
</style>