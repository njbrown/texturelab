<template>
  <div class="field">
    <label>{{prop.displayName}}</label>
    <input type="checkbox" :value="prop.value" @input="updateValue" />
  </div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit } from "vue-property-decorator";
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

  updateValue(evt) {
    this.node.setProperty(this.prop.name, evt.target.value);
    this.propertyChanged();
  }
}
</script>
