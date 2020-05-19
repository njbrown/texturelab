<template>
	<div class="field">
		<label>{{ prop.displayName }}</label>
		<div>
			<input
				class="color"
				type="color"
				:value="prop.value.toHex()"
				@input="updateValue"
			/>
		</div>
	</div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit } from "vue-property-decorator";
import { Designer } from "@/lib/designer";
import { DesignerNode } from "@/lib/designer/designernode";
import { IPropertyHolder } from "@/lib/designer/properties";

@Component
export default class ColorPropertyView extends Vue {
	@Prop()
	// ColorProperty
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

.color {
	margin-top: 0.4em;
	width: 100%;
	width: calc(100% - 4px - 1px);
}
</style>
