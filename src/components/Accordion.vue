<template>
	<div class="container">
		<div
			@click="
				collapse();
				$event.preventDefault();
			"
			class="header disable-select"
		>
			{{ isCollapsed ? "▼" : "▶" }} {{ header }}
		</div>
		<div v-show="isCollapsed" class="contents">
			<slot></slot>
		</div>
	</div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit, Model } from "vue-property-decorator";

@Component
export default class Accordion extends Vue {
	@Prop({
		default: "Header"
	})
	header: string;

	@Prop({
		default: false
	})
	collapsed: boolean;

	showContents: boolean = false;

	mounted() {
		this.showContents = this.collapsed;
	}

	collapse() {
		this.showContents = !this.showContents;
		console.log(this.showContents);
		console.log(this.isCollapsed);
	}

	get isCollapsed() {
		return !this.showContents;
	}
}
</script>

<style scoped>
.container {
	margin-bottom: 1px;
}
.header {
	color: white;
	font-weight: bold;
	font-size: 14px;
	background: #666;
	padding: 0.5em;
	padding-left: 0.5em;
	cursor: pointer;
}

.contents {
	margin-left: 0.5em;
	margin-right: 0.5em;
}

.disable-select {
	user-select: none; /* supported by Chrome and Opera */
	-webkit-user-select: none; /* Safari */
	-khtml-user-select: none; /* Konqueror HTML */
	-moz-user-select: none; /* Firefox */
	-ms-user-select: none; /* Internet Explorer/Edge */
}
</style>
