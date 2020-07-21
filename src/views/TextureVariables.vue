<template>
	<form class="ui form">
		<component
			v-for="(p, index) in this.properties"
			:is="p.componentName"
			:prop="p.var.property"
			:node="node"
			:editor="editor"
			@propertyChanged="propertyChanged"
			:key="index"
		></component>
	</form>
</template>

<script lang="ts">
import { Vue, Model, Prop, Component } from "vue-property-decorator";
import FloatVariableView from "@/components/variables/FloatVar.vue";
import BoolVariableView from "@/components/variables/BoolVar.vue";
import EnumVariableView from "@/components/variables/EnumVar.vue";
import ColorVariableView from "@/components/variables/ColorVar.vue";
import { Editor } from "@/lib/editor";
import { DesignerVariable } from "@/lib/designer/designervariable";
import { DesignerNode } from "@/lib/designer/designernode";
import { Designer } from "@/lib/designer";

class VarHolder {
	var: DesignerVariable;
	componentName: string;
}

@Component({
	components: {
		float: FloatVariableView,
		int: FloatVariableView,
		bool: BoolVariableView,
		enum: EnumVariableView,
		color: ColorVariableView
	}
})
export default class TextureVariablesView extends Vue {
	@Prop()
	node: DesignerNode;

	@Prop()
	designer: Designer;

	propertyChanged(propName: string) {
		// if (this.editor.onnodepropertychanged)
		//   this.editor.onnodepropertychanged(self.node, prop);
	}

	// calculated
	get properties(): VarHolder[] {
		let vars: VarHolder[] = this.designer.variables.map(v => {
			let name: string = "";
			return {
				var: v,
				componentName: name
			};
		});

		//console.log(vars);
		return vars;
	}
}
</script>
