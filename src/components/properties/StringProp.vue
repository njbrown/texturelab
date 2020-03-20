<template>
	<div class="field">
		<div>
			<label>{{ prop.displayName }}</label>
		</div>
		<div class="input-holder">
			<div style="width:95%; margin-right:10px">
				<textarea
					:value="prop.value"
					@input="updateValue"
					style="width:100%"
					rows="5"
				></textarea>
			</div>
		</div>
	</div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit } from "vue-property-decorator";
import { Designer } from "@/lib/designer";
import { DesignerNode } from "@/lib/designer/designernode";
import { IPropertyHolder } from "../../lib/designer/properties";

@Component
export default class StringPropertyView extends Vue {
	@Prop()
	// FloatProperty
	prop: any;

	@Prop()
	designer: Designer;

	@Prop()
	propHolder: IPropertyHolder;

	@Emit()
	propertyChanged() {
		return this.prop.name;
	}

	updateValue(evt) {
		this.propHolder.setProperty(this.prop.name, evt.target.value);
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

.number {
	width: calc(100% - 4px - 1px);
	border: solid gray 1px;
	padding: 2px;
	border-radius: 2px;
}

.input-holder {
	display: flex;
}
</style>
