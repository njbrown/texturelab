,
<template>
	<div class="field">
		<label>{{ prop.displayName }}</label>
		<div>
			<!-- <input
				class="color"
				type="color"
				:value="prop.value.toHex()"
				@input="onInput"
				@change="onValue"
			/> -->
			<color-picker
				:value="prop.value.toHex()"
				@input="onInput"
				@change="onValue"
			/>
		</div>
	</div>
</template>

<script lang="ts">
// https://codepen.io/Brownsugar/pen/NaGPKy
import { Vue, Prop, Component, Emit } from "vue-property-decorator";
import { Designer } from "@/lib/designer";
import { DesignerNode } from "@/lib/designer/designernode";
import { IPropertyHolder } from "@/lib/designer/properties";
import ColorPicker from "@/components/ColorPicker.vue";
import { PropertyChangeAction } from "@/lib/actions/propertychangeaction";
import { UndoStack } from "@/lib/undostack";

@Component({
	components: {
		ColorPicker
	}
})
export default class ColorPropertyView extends Vue {
	@Prop()
	// ColorProperty
	prop: any;

	oldValue: string;

	@Prop()
	designer: Designer;

	@Prop()
	propHolder: IPropertyHolder;

	public ColorPropertyView() {}

	mounted() {
		this.oldValue = this.prop.value.toHex();
	}

	@Emit()
	propertyChanged() {
		return this.prop.name;
	}

	onInput(value) {
		//console.log(value);
		this.propHolder.setProperty(this.prop.name, value);
		//this.propertyChanged();
	}

	onValue(value) {
		//let oldValue = this.prop.value.toHex();
		this.propHolder.setProperty(this.prop.name, value);

		let action = new PropertyChangeAction(
			null,
			this.prop.name,
			this.propHolder,
			this.oldValue,
			this.prop.value.toHex()
		);
		UndoStack.current.push(action);

		// console.log(
		// 	"color change: " + this.oldValue + " > " + this.prop.value.toHex()
		// );

		this.oldValue = this.prop.value.toHex();

		this.propertyChanged();
	}

	// onInput(evt) {
	// 	this.propHolder.setProperty(this.prop.name, evt.target.value);
	// 	//this.propertyChanged();
	// }

	// onValue(evt) {
	// 	//let oldValue = this.prop.value.toHex();
	// 	this.propHolder.setProperty(this.prop.name, evt.target.value);

	// 	let action = new PropertyChangeAction(
	// 		null,
	// 		this.prop.name,
	// 		this.propHolder,
	// 		this.oldValue,
	// 		this.prop.value.toHex()
	// 	);
	// 	UndoStack.current.push(action);

	// 	console.log(
	// 		"color change: " + this.oldValue + " > " + this.prop.value.toHex()
	// 	);

	// 	this.oldValue = this.prop.value.toHex();

	// 	this.propertyChanged();
	// }
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
	appearance: none;
}

.color::-webkit-color-swatch-wrapper {
	padding: 0;
}

.color::-webkit-color-swatch {
	border: none;
}
</style>
