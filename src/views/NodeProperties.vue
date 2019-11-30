<template>
	<form
		class="properties"
		@submit.prevent="cancelSubmit"
		:key="node.id"
		v-if="node != null"
	>
		<accordion header="Base Properties">
			<texture-channel :node="node" :editor="editor" />
		</accordion>
		<accordion header="Properties">
			<component
				v-for="(p, index) in this.properties"
				:is="p.componentName"
				:prop="p.prop"
				:node="node"
				:editor="editor"
				@propertyChanged="propertyChanged"
				:key="index"
			></component>
		</accordion>
	</form>
</template>

<script lang="ts">
import { Vue, Model, Prop, Component } from "vue-property-decorator";
import FloatPropertyView from "@/components/properties/FloatProp.vue";
import BoolPropertyView from "@/components/properties/BoolProp.vue";
import EnumPropertyView from "@/components/properties/EnumProp.vue";
import ColorPropertyView from "@/components/properties/ColorProp.vue";
import TextureChannelPropertyView from "@/components/properties/TextureChannelProp.vue";
import Accordion from "@/components/Accordion.vue";
import { Editor } from "@/lib/editortest";
import { DesignerNode } from "@/lib/designer/designernode";
import { Property } from "@/lib/designer/properties";
import GradientPropertyView from "@/components/properties/GradientProp.vue";

class PropHolder {
	prop: Property;
	componentName: string;
}

@Component({
	components: {
		float: FloatPropertyView,
		int: FloatPropertyView,
		bool: BoolPropertyView,
		enum: EnumPropertyView,
		color: ColorPropertyView,
		gradient: GradientPropertyView,

		textureChannel: TextureChannelPropertyView,
		Accordion
	}
})
export default class NodePropertiesView extends Vue {
	@Prop()
	node: DesignerNode;

	@Prop()
	editor: Editor;

	propertyChanged(propName: string) {
		// if (this.editor.onnodepropertychanged)
		//   this.editor.onnodepropertychanged(self.node, prop);
	}

	cancelSubmit() {
		return false;
	}

	// calculated
	get properties(): PropHolder[] {
		let props: PropHolder[] = this.node.properties.map(prop => {
			//let name: string = "";
			return {
				prop: prop,
				componentName: prop.type
			};
		});

		//console.log(props);
		return props;
	}
}
</script>

<style scoped>
.properties {
	background: #333;
}
</style>
