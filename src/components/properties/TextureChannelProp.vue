<template>
	<div class="field">
		<label>Texture Channel</label>
		<div :key="key">
			<select class="enum" @change="updateValue">
				<option
					v-for="(opt, index) in channelNames"
					:value="index"
					:key="index"
					:selected="index == channelIndex"
					>{{ opt }}</option
				>
			</select>
		</div>
	</div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit, Watch } from "vue-property-decorator";
import { Designer } from "@/lib/designer";
import { DesignerNode } from "@/lib/designer/designernode";
import { Editor } from "@/lib/editor";
import { ChangeTextureChannelAction } from "../../lib/actions/changetexturechannelaction";
import { UndoStack } from "../../lib/undostack";

@Component
export default class TextureChannelPropertyView extends Vue {
	channelNames = [
		"none",
		"albedo",
		"normal",
		"metalness",
		"roughness",
		"height",
		"emission",
		"ao",
		"alpha"
	];
	channelIndex: number = 0;
	key: number = 0;

	@Prop()
	editor: Editor;

	@Prop()
	node: DesignerNode;

	mounted() {
		this.calculateChannelIndex();
	}

	calculateChannelIndex() {
		this.channelIndex = 0;
		for (let channelName of this.editor.textureChannels.keys()) {
			if (this.editor.textureChannels.get(channelName) == null) continue;

			if (this.editor.textureChannels.get(channelName).id == this.node.id) {
				this.channelIndex = this.channelNames.indexOf(channelName);
			}
		}

		this.key += 1;
	}

	updateValue(evt) {
		let index = evt.currentTarget.value;
		//set channel in editor
		let channelName = this.channelNames[index];

		//fetch params for undo/redo
		let node2 = null;
		let channel2 = index == 0 ? null : channelName;

		let nodeView = this.editor.graph.getNodeById(this.node.id);
		let channel1 = nodeView.textureChannel;
		if (this.editor.textureChannels.has(channelName))
			node2 = this.editor.textureChannels.get(channelName).id;
		let node1 = this.node.id;

		if (index != 0) {
			this.editor.assignNodeToTextureChannel(this.node.id, channelName);
		} else {
			this.editor.clearTextureChannel(this.node.id);
		}

		let action = new ChangeTextureChannelAction(
			() => this.calculateChannelIndex(),
			this.editor,
			node1,
			channel1,
			node2,
			channel2
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

.enum {
	outline: 0;
	box-shadow: none;
	border: 0 !important;

	margin-top: 0.4em;
	width: 100%;
	border: none;
	border-radius: 4px;
	color: white;
	background: #222;
	padding: 0.5em;
	font-family: "Open Sans";
}
</style>
