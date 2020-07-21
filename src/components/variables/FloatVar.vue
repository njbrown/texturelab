<template>
	<div class="field">
		<label>{{ prop.displayName }}</label>
		<input
			type="range"
			:min="prop.minValue"
			:max="prop.maxValue"
			:value="prop.value"
			:step="prop.step"
			@input="updateValue"
			class="slider"
		/>
	</div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit } from "vue-property-decorator";
import { Designer } from "@/lib/designer";
import { DesignerNode } from "@/lib/designer/designernode";

@Component
export default class FloatVariableView extends Vue {
	@Prop()
	// FloatProperty
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

<style scoped>
/* https://www.w3schools.com/howto/howto_js_rangeslider.asp */
/* http://jsfiddle.net/brenna/f4uq9edL/?utm_source=website&utm_medium=embed&utm_campaign=f4uq9edL */
.slider {
	-webkit-appearance: none;
	width: 100%;
	height: 4px;
	border-radius: 4px;
	background-color: rgba(0, 0, 0);
	color: rgba(0, 0, 0);
	outline: none;
	-webkit-transition: 0.2s;
	transition: opacity 0.2s;
}

.slider::-webkit-slider-thumb {
	-webkit-appearance: none;
	appearance: none;
	width: 20px;
	height: 20px;
	border-radius: 50%;
	background: #fff -webkit-linear-gradient(transparent, rgba(0, 0, 0, 0.05));
	cursor: pointer;
	box-shadow: 0 1px 2px 0 rgba(34, 36, 38, 0.15),
		0 0 0 1px rgba(34, 36, 38, 0.15) inset;
}

.slider::-moz-range-thumb {
	width: 25px;
	height: 25px;
	border-radius: 50%;
	background: #fff -webkit-linear-gradient(transparent, rgba(0, 0, 0, 0.05));
	cursor: pointer;
	box-shadow: 0 1px 2px 0 rgba(34, 36, 38, 0.15),
		0 0 0 1px rgba(34, 36, 38, 0.15) inset;
}

.slider::-ms-thumb {
	min-height: 20px;
	transform: scale(1) !important;
	width: 25px;
	height: 25px;
	border-radius: 50%;
}

.slider::-ms-fill-lower {
	background: #777;
	border-radius: 10px;
}

.slider::-ms-fill-upper {
	background: #ddd;
	border-radius: 10px;
}

.texture-options {
	background: #e0e0e0;
	border-radius: 3px;
	margin-bottom: 1em !important;
	padding: 1em;
}
</style>
