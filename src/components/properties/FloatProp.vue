<template>
	<div class="field">
		<div>
			<label>{{ prop.displayName }}</label>
		</div>
		<div class="input-holder">
			<div style="width:85%; margin-right:10px">
				<input
					type="range"
					:min="prop.minValue"
					:max="prop.maxValue"
					:value="prop.value"
					:step="prop.step"
					@input="updateValue"
					class="slider"
					@focus="focus"
					@blur="blur"
				/>
			</div>
			<div style="width:15%">
				<input
					type="number"
					:value="prop.value"
					:step="prop.step"
					@input="updateValue"
					class="number"
					@focus="focus"
					@blur="blur"
				/>
			</div>
		</div>
	</div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit } from "vue-property-decorator";
import { Designer } from "@/lib/designer";
import { DesignerNode } from "@/lib/designer/designernode";
import { IPropertyHolder } from "../../lib/designer/properties";
import { PropertyChangeComplete } from "./ipropertyui";
import { UndoStack } from "@/lib/undostack";
import { PropertyChangeAction } from "@/lib/actions/propertychangeaction";

@Component
export default class FloatPropertyView extends Vue {
	@Prop()
	// FloatProperty
	prop: any;

	@Prop()
	designer: Designer;

	@Prop()
	propHolder: IPropertyHolder;

	oldValue: number;

	@Emit()
	propertyChanged() {
		return this.prop.name;
	}

	@Emit()
	propertyChangeCompleted(evt: PropertyChangeComplete) {
		return evt;
	}

	updateValue(evt) {
		this.propHolder.setProperty(this.prop.name, evt.target.value);
		this.propertyChanged();
	}

	focus() {
		//console.log("focus");
		this.oldValue = this.prop.value;
	}

	blur() {
		//console.log("blur");
		// let evt = {
		// 	propName: this.prop.name,
		// 	oldValue: this.oldValue,
		// 	newValue: this.prop.value,
		// };
		// this.propertyChangeCompleted(evt);
		let action = new PropertyChangeAction(
			null,
			this.prop.name,
			this.propHolder,
			this.oldValue,
			this.prop.value
		);
		UndoStack.current.push(action);
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

/* https://www.w3schools.com/howto/howto_js_rangeslider.asp */
/* http://jsfiddle.net/brenna/f4uq9edL/?utm_source=website&utm_medium=embed&utm_campaign=f4uq9edL */
.slider {
	-webkit-appearance: none;
	width: 100%;
	height: 3px;
	border-radius: 4px;
	background-color: rgb(255, 255, 255, 0.7);
	color: rgba(0, 0, 0);
	outline: none;
	-webkit-transition: 0.2s;
	transition: opacity 0.2s;
}

.slider::-webkit-slider-thumb {
	-webkit-appearance: none;
	appearance: none;
	width: 15px;
	height: 15px;
	border-radius: 50%;
	background: #fff -webkit-linear-gradient(transparent, rgba(0, 0, 0, 0.05));
	cursor: pointer;
	box-shadow: 0 1px 2px 0 rgba(34, 36, 38, 0.15),
		0 0 0 1px rgba(34, 36, 38, 0.15) inset;
}

.slider::-moz-range-thumb {
	width: 10px;
	height: 10px;
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
