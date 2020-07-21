<template>
	<div class="input-group color-picker" ref="colorpicker">
		<!-- <input
			type="text"
			class="form-control"
			v-model="colorValue"
			@focus="showPicker()"
			ref="input"
    />-->
		<span class="input-group-addon color-picker-container">
			<span
				ref="span"
				class="color-ui"
				:style="'background-color: ' + colorValue"
				@click="togglePicker()"
			></span>
			<sketch
				:value="colors"
				@input="updateFromPicker"
				v-if="displayPicker"
				ref="sketch"
			/>
		</span>
	</div>
</template>

<style scoped>
h1 {
	height: 120px;
	line-height: 120px;
	text-align: center;
}
.vc-chrome {
	position: absolute;
	top: 35px;
	right: 0;
	z-index: 9;
}
.color-ui {
	display: inline-block;
	width: 95%;
	height: 2.5em;
	background-color: #000;
	cursor: pointer;
	border: solid white 3px;
	border-radius: 4px;
}
.footer {
	margin-top: 20px;
	text-align: center;
}
</style>

<script lang="ts">
// https://codepen.io/Brownsugar/pen/NaGPKy
import { Vue, Prop, Component, Emit, Watch } from "vue-property-decorator";
import { Designer } from "@/lib/designer";
import { DesignerNode } from "@/lib/designer/designernode";
import { IPropertyHolder } from "@/lib/designer/properties";
import { Sketch } from "vue-color";
import { Color } from "../lib/designer/color";

@Component({
	components: {
		Sketch
	}
})
export default class ColorPicker extends Vue {
	@Prop()
	value: string;

	oldValue: string;

	colors: any = {};

	colorValue: string = "";
	displayPicker: boolean = false;

	// @Emit()
	// propertyChanged() {
	// 	return this.prop.name;
	// }

	// updateValue(evt) {
	// 	this.propHolder.setProperty(this.prop.name, evt.target.value);
	// 	this.propertyChanged();
	// }

	mounted() {
		this.setColor(this.value);
		this.oldValue = this.value;
	}

	setColor(color) {
		//this.updateColors(color);
		this.colorValue = color;
	}

	// updateColors(color) {
	// 	if (color.slice(0, 1) == "#") {
	// 		this.colors = {
	// 			hex: color,
	// 		};
	// 	} else if (color.slice(0, 4) == "rgba") {
	// 		var rgba = color.replace(/^rgba?\(|\s+|\)$/g, "").split(","),
	// 			hex =
	// 				"#" +
	// 				(
	// 					(1 << 24) +
	// 					(parseInt(rgba[0]) << 16) +
	// 					(parseInt(rgba[1]) << 8) +
	// 					parseInt(rgba[2])
	// 				)
	// 					.toString(16)
	// 					.slice(1);
	// 		this.colors = {
	// 			hex: hex,
	// 			a: rgba[3],
	// 		};
	// 	}
	// }

	showPicker() {
		document.addEventListener("click", this.documentClick);
		this.displayPicker = true;
	}
	hidePicker() {
		document.removeEventListener("click", this.documentClick);
		this.displayPicker = false;

		if (this.colorValue != this.oldValue) {
			this.oldValue = this.colorValue;
			this.emitChange(this.colorValue);
		}
	}
	togglePicker() {
		this.displayPicker ? this.hidePicker() : this.showPicker();
	}
	updateFromPicker(color) {
		this.colors = color;
		this.colorValue = color.hex;
	}
	documentClick(e) {
		let el = this.$refs.colorpicker,
			target = e.target;
		let sketch = (this.$refs.sketch as Vue).$el;

		if (
			target !== el &&
			target !== this.$refs.span &&
			target !== this.$refs.input &&
			!sketch.contains(target)
		) {
			this.hidePicker();
		}
	}

	@Emit("input")
	emitInput(value: any) {
		return value;
	}

	@Emit("change")
	emitChange(value: any) {
		return value;
	}

	@Watch("colorValue")
	colorValueChanged(newValue) {
		this.emitInput(newValue);
	}
}
</script>
