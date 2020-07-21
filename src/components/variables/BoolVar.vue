<template>
	<div class="field">
		<label>{{ prop.displayName }}</label>
		<input type="checkbox" :value="prop.value" @input="updateValue" />
	</div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit } from "vue-property-decorator";
import { Designer } from "@/lib/designer";
import { DesignerNode } from "@/lib/designer/designernode";

@Component
export default class BoolVariableView extends Vue {
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
		this.designer.setVariable(this.prop.name, evt.target.value);
		this.propertyChanged();
	}
}
</script>
