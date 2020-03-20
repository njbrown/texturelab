<template>
	<form
		class="properties"
		@submit.prevent="cancelSubmit"
		:key="node.id"
		v-if="node != null"
	>
		<accordion header="Base Properties" v-if="isInstanceNode">
			<texture-channel :node="getNode" :editor="editor" />
		</accordion>
		<accordion header="Properties">
			<component
				v-for="(p, index) in this.properties"
				:is="p.componentName"
				:prop="p.prop"
				:propHolder="node"
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
import { Property, IPropertyHolder } from "@/lib/designer/properties";
import GradientPropertyView from "@/components/properties/GradientProp.vue";
import StringPropertyView from "@/components/properties/StringProp.vue";

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
		string: StringPropertyView,

		textureChannel: TextureChannelPropertyView,
		Accordion
	}
})
export default class NodePropertiesView extends Vue {
	@Prop()
	node: IPropertyHolder;

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

	get isInstanceNode() {
		return this.node instanceof DesignerNode;
	}

	get getNode(): DesignerNode {
		return <DesignerNode>this.node;
	}
}
</script>

<style scoped>
.properties {
	background: #333;
}
</style>
