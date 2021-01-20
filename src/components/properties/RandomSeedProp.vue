<template>
	<div class="field">
		<div>
			<label>Random Seed</label>
		</div>
		<div class="input-holder">
			<div style="width:100%; margin-right:10px;padding:0.4em;">
				<input
					type="range"
					:min="0"
					:max="50"
					:value="randomSeed"
					:step="1"
					@input="updateValue"
					class="slider"
					@mousedown="focus"
					@mouseup="blur"
				/>
			</div>
			<div style="width:70px;">
				<input
					type="number"
					:value="randomSeed"
					:step="1"
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
import { UndoStack } from "@/lib/undostack";
import { SetNodeRandomSeedAction } from "@/lib/actions/setnoderandomseedaction";

@Component
export default class RandomSeedPropertyView extends Vue {

	@Prop()
	node: DesignerNode;

	randomSeed:number = 0;
	oldValue:number = 0;

	mounted() {
		this.randomSeed = this.node.randomSeed;
		this.oldValue = this.randomSeed;
	}

	updateValue(evt) {
		this.node.setRandomSeed(evt.target.value);
		this.randomSeed = evt.target.value;
	}

	// update called after undo action
	undoUpdate() {
		this.randomSeed = this.node.randomSeed;
	}

	focus() {
		console.log("focus");
		this.oldValue = this.randomSeed;
	}

	blur() {
		console.log("blur");

		let action = new SetNodeRandomSeedAction(
			()=>{this.undoUpdate();},
			this.node,
			this.oldValue,
			this.randomSeed
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
	width: calc(100% - 1em - 1px);
	border: solid transparent 1px;
	border-radius: 4px;
	position: relative;
	outline: none;

	background: #4e4e4e;
	color: rgba(255, 255, 255, 0.8);
	padding: 0.5em;
}

.number:focus {
	border-color: dodgerblue;
}

.number::-webkit-inner-spin-button {
	width: 1em;
	border-left: 1px solid #bbb;
	opacity: 1;
	color: rgb(130, 130, 130);
	position: absolute;
	top: 0;
	right: 0;
	bottom: 0;
	cursor: pointer;
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
	border-radius: 5px;
	background-color: rgb(255, 255, 255, 0.7);
	color: rgba(0, 0, 0);
	outline: none;
	-webkit-transition: 0.2s;
	transition: opacity 0.2s;
}

.slider::-webkit-slider-thumb {
	-webkit-appearance: none;
	appearance: none;
	width: 17px;
	height: 17px;
	border-radius: 50%;
	/* background: #fff -webkit-linear-gradient(transparent, rgba(0, 0, 0, 0.05)); */
	background-color: rgb(51, 51, 51);
	border: solid white 2px;
	outline: solid rgb(51, 51, 51) 3px;
	cursor: pointer !important;
	/* box-shadow: 0 1px 2px 0 rgba(34, 36, 38, 0.15),
    0 0 0 1px rgba(34, 36, 38, 0.15) inset; */
}

.slider::-moz-range-thumb {
	width: 10px;
	height: 10px;
	border-radius: 50%;
	background-color: rgb(51, 51, 51);
	border: solid white 2px;
	outline: solid rgb(51, 51, 51) 3px;
	cursor: pointer !important;
	box-shadow: 0 1px 2px 0 rgba(34, 36, 38, 0.15),
		0 0 0 1px rgba(34, 36, 38, 0.15) inset;
}

.slider::-ms-thumb {
	min-height: 20px;
	transform: scale(1) !important;
	width: 25px;
	height: 25px;
	border-radius: 50%;
	background-color: rgb(51, 51, 51);
	border: solid white 2px;
	outline: solid rgb(51, 51, 51) 3px;
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
