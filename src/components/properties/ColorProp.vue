<template>
  <div class="field">
    <label>{{prop.displayName}}</label>
    <input type="color" :value="prop.value.toHex()" @input="updateValue" style="width:100%" />
  </div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit } from "vue-property-decorator";
import { Designer, DesignerNode } from "@/lib/nodetest";

@Component
export default class ColorPropertyView extends Vue {
  @Prop()
  // ColorProperty
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
