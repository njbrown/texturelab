<template>
	<div class="field">
		<label>{{ prop.displayName }}</label>
		<select @change="updateValue">
			<option
				v-for="(opt, index) in prop.values"
				:value="index"
				:key="index"
				:selected="index == prop.index"
				>{{ opt }}</option
			>
		</select>
	</div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit } from "vue-property-decorator";
import { Designer } from "@/lib/designer";
import { DesignerNode } from "@/lib/designer/designernode";

@Component
export default class EnumVariableView extends Vue {
	@Prop()
	// EnumProperty
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
