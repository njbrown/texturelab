<template>
	<vue-final-modal
		:value="exportMenuVisible"
		classes="modal-container"
		content-class="modal-content"
	>
		<button class="modal__close" @click="hideExportMenu()">
			<i class="bx bx-x" style="font-size:1.4rem !important;"></i>
		</button>
		<span class="modal__title">Export Settings</span>
		<!-- <div>
					<span>Output Type</span>
					<select>
						<option value="zip">Zip</option>
						<option value="folder">Folder</option>
					</select>
				</div> -->
		<div class="modal__content">
			<div class="prop-container">
				<span class="prop-title">Destination:</span><br />
				<span>{{ exportDestination }}</span
				><a
					class="small-button"
					href="#"
					v-if="exportDestination"
					@click="chooseExportDestination()"
				>
					...
				</a>
				<a
					class="small-button"
					href="#"
					v-else
					@click="chooseExportDestination()"
				>
					Choose Folder
				</a>
			</div>
			<div class="prop-container">
				<span class="prop-title">Pattern:</span><br />
				<div>
					<input
						class="text-input"
						:value="exportPattern"
						@change="onExportPatternChanged"
					/>
					<a class="small-button" href="#" @click="resetExportPattern()"
						>reset</a
					>
				</div>
				<div style="font-size:0.7em;">
					${project} - Project Name <br />
					${name} - Output Node Name <br />
				</div>
			</div>
			<div>
				<a
					class="button"
					style="float:right;margin-top:1em;"
					href="#"
					@click="exportTextures()"
				>
					Export
				</a>
			</div>
		</div>
	</vue-final-modal>
</template>

<style scoped>
::v-deep .modal-container {
	display: flex;
	justify-content: center;
	align-items: center;
	background: rgba(0, 0, 0, 0.5);
}
::v-deep .modal-content {
	position: relative;
	display: flex;
	flex-direction: column;
	margin: 0 1rem;
	padding: 1rem;
	/* border: 1px solid #e2e8f0; */
	border-radius: 0.25rem;
	background: #333;
	color: white;
	min-width: 300px;
}
.modal__title {
	margin: 0 2rem 0 0;
	font-size: 1.5rem;
	font-weight: 700;
}
.modal__close {
	position: absolute;
	top: 0.5rem;
	right: 0.5rem;
}

.button {
	border-radius: 2px;
	background: #666;
	padding: 0.5em 1em;
	text-decoration: none;
	color: white;
	vertical-align: middle;
	margin-right: 0.5em;
}

.small-button {
	border-radius: 2px;
	background: #666;
	padding: 0.2em 0.5em;
	text-decoration: none;
	color: white;
	vertical-align: middle;
	margin-right: 0.5em;
}

.prop-container {
	margin-top: 0.5em;
}

.prop-title {
	font-size: 0.8em;
}

.text-input {
	padding: 0.4em;
	border: solid white 1px;
	border-radius: 2px;
	margin-right: 1em;
}
</style>

<script lang="ts">
import { Component, Prop, Vue, Watch, Model } from "vue-property-decorator";
import VfmPlugin from "vue-final-modal";
import { Exporter, ExportSettings, OutputType } from "@/export";
import { dialog, shell } from "@electron/remote";
import { Project } from "@/lib/project";
import { Editor } from "@/lib/editor";
const remote = require("@electron/remote");

@Component({})
export default class ExportDialog extends Vue {
	@Prop()
	exportMenuVisible: boolean = false;

	exportDestination: string = null;

	@Prop()
	exportPattern: string = "${project}_${name}";
	// use channel name when no export name is present
	// this is a fallback for existing textures made prior
	// to the new export system
	exportUseChannelName: boolean = false;

	@Prop()
	project: Project;

	@Prop()
	editor: Editor;

	mounted() {
		console.log("export dialog mounted");
	}

	public created(): void {
		// alert("mounted!");
		console.log("mounted");
		// this.exportMenuVisible = false;
	}

	showExportMenu() {
		this.$emit("show");
		// this.exportMenuVisible = true;
	}

	hideExportMenu() {
		this.$emit("hide");
		// this.exportMenuVisible = false;
	}

	onExportPatternChanged(event: MouseEvent) {
		// this.exportPattern = value;
		this.$emit(
			"exportPatternChanged",
			(event.target as HTMLInputElement).value
		);
	}

	chooseExportDestination() {
		let path = dialog.showOpenDialogSync(remote.getCurrentWindow(), {
			properties: ["openDirectory", "createDirectory"]
		});

		console.log(path);
		if (path && path.length > 0) {
			this.exportDestination = path[0];
		}
	}

	resetExportPattern() {
		this.exportPattern = "${project}_${name}";
	}

	exportTextures() {
		this.$emit("exportTextures");
	}
}
</script>
