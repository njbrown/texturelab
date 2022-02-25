<template>
	<canvas
		style="width:10em;border-radius:4px;height:1.7em;margin:2px;"
		ref="canvas"
		width="160"
		height="27"
	></canvas>
</template>

<script lang="ts">
import { Component, Prop, Vue, Watch } from "vue-property-decorator";

declare var __static: any;

@Component
export default class LibraryMenu extends Vue {
	@Prop()
	total: number;

	@Prop()
	value: number;

	context: CanvasRenderingContext2D;
	canvas: HTMLCanvasElement;

	mounted() {
		console.log(this.$refs);
		const canvas = this.$refs.canvas as HTMLCanvasElement;
		this.context = canvas.getContext("2d");
		this.canvas = canvas;

		// console.log(rect);

		requestAnimationFrame(() => {
			this.renderBar();
		});
	}

	beforeUpdate() {
		requestAnimationFrame(() => {
			this.renderBar();
		});
	}

	@Watch("value")
	valueUpdated() {
		this.canvas.width = this.canvas.clientWidth;
		this.canvas.height = this.canvas.clientHeight;
		const style = getComputedStyle(this.canvas);

		console.log(this.canvas.clientWidth);
		console.log(this.canvas.offsetWidth);
		console.log(parseInt(style.width));
		console.log(this.canvas.clientHeight);

		this.renderBar();
	}

	renderBar() {
		const width = this.canvas.width;
		const height = this.canvas.height;
		const ctx = this.context;

		ctx.setTransform(1, 0, 0, 1, 0, 0);
		ctx.clearRect(0, 0, width, height);

		ctx.fillStyle = "black";
		ctx.fillRect(0, 0, width, height);
		console.log("rendering bar");

		console.log("total: " + this.total);
		console.log("value: " + this.value);

		if (this.total != 0) {
			const percent = Math.round((this.value / this.total) * 100);
			console.log("percent: " + percent);

			ctx.fillStyle = "#1b7dd9";
			ctx.fillRect(0, 0, width * (this.value / this.total), height);

			// draw centered text
			ctx.font = "bold 14px 'Open Sans'";
			ctx.fillStyle = "white";
			const text = `${percent}% (${this.value}/${this.total})`;
			const size = ctx.measureText(text);
			const textX = width / 2 - size.width / 2;
			const textY = height / 2 + 7.0 / 2;
			ctx.fillText(text, textX, textY);
		}
	}
}
</script>

<style></style>
