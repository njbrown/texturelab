<template>
	<div class="field">
		<div>
			<label>{{ prop.displayName }}</label>
		</div>
		<div class="input-holder" style="min-height:50px;" ref="inputHolder">
			<canvas ref="canvas" />
		</div>
	</div>
</template>

<script lang="ts">
import { Vue, Prop, Component, Emit, Watch } from "vue-property-decorator";
import { Designer } from "@/lib/designer";
import { DesignerNode } from "@/lib/designer/designernode";
import { Gradient, GradientPoint } from "@/lib/designer/gradient";
import { Color } from "@/lib/designer/color";
import elementResizeDetectorMaker from "element-resize-detector";
import { IPropertyHolder } from "@/lib/designer/properties";
import { PropertyChangeAction } from "@/lib/actions/propertychangeaction";
import { UndoStack } from "@/lib/undostack";

@Component
export default class GradientPropertyView extends Vue {
	@Prop()
	// FloatProperty
	prop: any;

	widget: GradientWidget;

	@Prop()
	designer: Designer;

	@Prop()
	propHolder: IPropertyHolder;

	oldValue: Gradient;

	mounted() {
		this.widget = new GradientWidget({
			width: (this.$refs.inputHolder as HTMLDivElement).offsetWidth,
			canvas: this.$refs.canvas
		});

		this.oldValue = this.prop.getValue().clone();
		this.widget.setGradient(this.prop.value.clone());
		this.widget.onvaluechanged = this.updateChanged;
		this.widget.oninput = this.updateInput;

		let erd = new elementResizeDetectorMaker();
		erd.listenTo(this.$refs.inputHolder, element => {
			var width = element.offsetWidth;
			var height = element.offsetHeight;

			this.widget.resize(width, height);
		});
	}

	beforeDestroy() {
		this.widget.dispose();
	}

	@Emit()
	propertyChanged() {
		return this.prop.name;
	}

	updateInput(gradient: Gradient) {
		this.propHolder.setProperty(this.prop.name, gradient.clone());
	}

	updateChanged(gradient: Gradient) {
		let newValue = gradient.clone();
		this.propHolder.setProperty(this.prop.name, gradient.clone());

		let action = new PropertyChangeAction(
			// todo: this is a bad fix, correct fix below
			() => {
				this.widget.setGradient(this.prop.getValue());
			},
			this.prop.name,
			this.propHolder,
			this.oldValue,
			newValue
		);
		UndoStack.current.push(action);

		this.oldValue = newValue.clone();
	}

	@Watch("prop", { deep: true })
	gradChanged(oldVal, newVal) {
		//console.log("grad changed");
		// if new val is different from widget val
		// then set gradient
		// otherwise do nothing
		// todo: this is the proper fix
	}
}

function clamp(val, min, max) {
	return Math.min(Math.max(val, min), max);
}

export class Point {
	constructor(public x: number, public y: number) {}
}

export class Box {
	public x: number = 0;
	public y: number = 0;
	public width: number = 1;
	public height: number = 1;

	public isPointInside(px: number, py: number): boolean {
		if (
			px >= this.x &&
			px <= this.x + this.width &&
			py >= this.y &&
			py <= this.y + this.height
		)
			return true;
		return false;
	}

	public setCenter(x: number, y: number) {
		this.x = x - this.width / 2;
		this.y = y - this.height / 2;
	}

	public setCenterX(x: number) {
		this.x = x - this.width / 2;
	}

	public setCenterY(y: number) {
		this.y = y - this.height / 2;
	}

	public centerX(): number {
		return this.x + this.width / 2;
	}

	public centerY(): number {
		return this.y + this.height / 2;
	}

	public move(dx: number, dy: number) {
		this.x += dx;
		this.y += dy;
	}
}

class GradientHandle {
	xbox: Box;
	colorBox: Box = new Box();

	gradientPoint: GradientPoint;
}

export class GradientWidget {
	canvas: HTMLCanvasElement;
	ctx: CanvasRenderingContext2D;
	gradient: Gradient;

	width: number;
	height: number;

	handles: GradientHandle[];
	lastMouseDown: Point;
	hitHandle: GradientHandle;
	// if the hitHandle is just created then this is true
	// it prevents the color picking showingup on mouse release
	// for new handles
	isNewHandle: boolean;

	handleSize: number;

	onvaluechanged: (gradient: Gradient) => void;
	oninput: (gradient: Gradient) => void;

	constructor(options: any) {
		this.width = options.width || 300;
		this.height = options.height || 50;
		this.handleSize = options.handleSize || 16;

		this.canvas = options.canvas || document.createElement("canvas");
		this.canvas.width = this.width;
		this.canvas.height = this.height;
		this.ctx = this.canvas.getContext("2d");

		this.handles = Array();
		this.lastMouseDown = new Point(0, 0);
		this.isNewHandle = false;

		//this.gradient = new Gradient();
		this.setGradient(new Gradient());
		this.bindEvents();
		this.redrawCanvas();
	}

	resize(width: number, height: number) {
		this.canvas.width = width;
		this.width = width;

		// recalc positions of all handles
		for (let handle of this.handles) {
			handle.colorBox.setCenterX(width * handle.gradientPoint.t);
		}

		this.redrawCanvas();
	}

	bindEvents() {
		var self = this;
		this.canvas.onmousedown = evt => this.onMouseDown(evt);

		//this.canvas.onmouseup = evt => this.onMouseUp(evt);
		document.documentElement.addEventListener("mouseup", evt =>
			self.onMouseUp(evt)
		);

		//this.canvas.onmousemove = this.onMouseMove;
		document.documentElement.addEventListener("mousemove", evt =>
			self.onMouseMove(evt)
		);

		this.canvas.oncontextmenu = function(evt: PointerEvent) {
			evt.preventDefault();
		};

		/*
        this.canvas.onmouseleave = function(evt:MouseEvent)
        {
            // cancel dragging
            self.hitHandle = null;
            self.redrawCanvas();
        }
        */
	}

	onMouseDown(evt: MouseEvent) {
		let self = this;

		self.lastMouseDown = getMousePos(self.canvas, evt);
		self.hitHandle = self.getHitHandle(self.lastMouseDown);
		self.isNewHandle = false;

		// if no box is hit then add one and make it the drag target
		if (self.hitHandle == null && evt.button == 0) {
			var t = self.lastMouseDown.x / self.width;
			var col = self.gradient.sample(t);
			var p = self.gradient.addPoint(t, col);

			var handle = self.createGradientHandleFromPoint(p);

			self.handles.push(handle);

			// make handle draggable
			self.hitHandle = handle;
			self.isNewHandle = true;

			if (self.oninput) self.oninput(self.gradient);
			//if (self.onvaluechanged) self.onvaluechanged(self.gradient);
		} else if (self.hitHandle && evt.button == 2) {
			// delete handle
			self.removeHandle(self.hitHandle);
			self.hitHandle = null;

			if (self.oninput) self.oninput(self.gradient);
			//if (self.onvaluechanged) self.onvaluechanged(self.gradient);
		}

		self.redrawCanvas();
	}

	onMouseUp(evt: MouseEvent) {
		let self = this;

		var hitPos = getMousePos(self.canvas, evt);
		if (
			self.lastMouseDown.x == hitPos.x &&
			self.lastMouseDown.y == hitPos.y &&
			self.hitHandle &&
			!self.isNewHandle
		) {
			let hitHandle = self.hitHandle;
			self.hitHandle = null;

			// show color picker
			var input = document.createElement("input");
			input.type = "color";
			input.value = hitHandle.gradientPoint.color.toHex();
			input.onchange = function(ev: Event) {
				console.log("onchange");
				console.log(ev);
				console.log(hitHandle);
				hitHandle.gradientPoint.color = Color.parse(input.value);
				self.redrawCanvas();
				//self.hitHandle = null;

				if (self.onvaluechanged) self.onvaluechanged(self.gradient);
			};

			input.oninput = (ev: Event) => {
				console.log(evt);
				console.log("oninput");

				hitHandle.gradientPoint.color = Color.parse(input.value);
				self.redrawCanvas();
				//self.hitHandle = null;

				if (self.oninput) self.oninput(self.gradient);
			};
			input.click();

			//input.onchange = null; // cleanup
			//self.hitHandle = null;
		} else if (self.hitHandle) {
			if (self.onvaluechanged) self.onvaluechanged(self.gradient);
			self.hitHandle = null;
		} else {
			self.hitHandle = null;
			self.redrawCanvas();
		}
	}

	onMouseMove(evt: MouseEvent) {
		let self = this;
		//var hitPos = getMousePos(self.canvas, evt);
		//console.log(self);
		var hitPos = getMousePos(self.canvas, evt);
		if (self.hitHandle) {
			// cap hit pos
			let x = clamp(hitPos.x, 0, self.width);
			self.hitHandle.colorBox.setCenterX(x);

			// recalc gradient t
			var t = x / self.width;
			self.hitHandle.gradientPoint.t = t;

			// resort handles
			self.gradient.sort();

			//if (self.onvaluechanged) self.onvaluechanged(self.gradient);
			if (self.oninput) self.oninput(self.gradient);
		}

		self.redrawCanvas();
	}

	removeHandle(handle: GradientHandle) {
		this.gradient.removePoint(handle.gradientPoint);
		this.handles.splice(this.handles.indexOf(handle), 1);
	}

	getHitHandle(pos: Point): GradientHandle {
		for (let handle of this.handles) {
			if (handle.colorBox.isPointInside(pos.x, pos.y)) {
				//console.log("handle hit!");
				return handle;
			}
		}

		return null;
	}

	setGradient(grad: Gradient) {
		this.handles = Array();
		var handleSize = this.handleSize;

		for (let p of grad.points) {
			var handle = this.createGradientHandleFromPoint(p);
			this.handles.push(handle);
		}

		this.gradient = grad;

		this.redrawCanvas();
	}

	createGradientHandleFromPoint(p: GradientPoint): GradientHandle {
		var handleSize = this.handleSize;

		var handle = new GradientHandle();
		handle.gradientPoint = p;
		// eval point locations
		var box = handle.colorBox;
		box.width = handleSize;
		box.height = handleSize;
		var x = p.t * this.width;
		box.setCenterX(x);
		box.y = this.height - handleSize;

		return handle;
	}

	public el() {
		return this.canvas;
	}

	redrawCanvas() {
		var ctx = this.ctx;

		ctx.clearRect(0, 0, this.width, this.height);

		var grad = ctx.createLinearGradient(0, 0, this.width, 0);
		for (let point of this.gradient.points) {
			grad.addColorStop(point.t, point.color.toHex());
		}

		ctx.fillStyle = grad;
		ctx.fillRect(0, 0, this.width, this.height - this.handleSize);

		this.drawHandles();
	}

	drawHandles() {
		var ctx = this.ctx;
		for (let handle of this.handles) {
			var colBox = handle.colorBox;
			// background
			ctx.beginPath();
			ctx.fillStyle = handle.gradientPoint.color.toHex();
			//ctx.rect(colBox.x, colBox.y, colBox.width, colBox.height);
			roundRect(ctx, colBox.x, colBox.y, colBox.width, colBox.height, 1);
			ctx.fill();

			// border
			ctx.beginPath();
			ctx.lineWidth = 2;
			ctx.strokeStyle = "rgb(0, 0, 0)";
			//ctx.rect(colBox.x, colBox.y, colBox.width, colBox.height);
			roundRect(ctx, colBox.x, colBox.y, colBox.width, colBox.height, 1);
			ctx.stroke();

			// triangle at top
			ctx.fillStyle = "rgb(0, 0, 0)";
			ctx.beginPath();
			ctx.moveTo(colBox.centerX(), colBox.y - 5);
			ctx.lineTo(colBox.x, colBox.y);
			ctx.lineTo(colBox.x + colBox.width, colBox.y);
			ctx.fill();
		}
	}

	dispose() {
		// remove all callbacks
	}
}

// https://www.html5canvastutorials.com/advanced/html5-canvas-mouse-coordinates/
// https://stackoverflow.com/questions/17130395/real-mouse-position-in-canvas
function getMousePos(canvas, evt) {
	var rect = canvas.getBoundingClientRect();
	return {
		x: evt.clientX - rect.left,
		y: evt.clientY - rect.top
	};
}

//https://stackoverflow.com/questions/10214873/make-canvas-as-wide-and-as-high-as-parent
function fitCanvasToContainer(canvas) {
	// Make it visually fill the positioned parent
	canvas.style.width = "100%";
	// 1em is the size of the top bar
	canvas.style.height = "100%";
	// ...then set the internal size to match
	canvas.width = canvas.offsetWidth;
	canvas.height = canvas.offsetHeight;
	//canvas.height = canvas.offsetWidth;

	canvas.style.width = "auto";
	canvas.style.height = "auto";
}

// https://stackoverflow.com/questions/1255512/how-to-draw-a-rounded-rectangle-on-html-canvas
function roundRect(ctx: CanvasRenderingContext2D, x, y, w, h, r) {
	if (w < 2 * r) r = w / 2;
	if (h < 2 * r) r = h / 2;
	ctx.beginPath();
	ctx.moveTo(x + r, y);
	ctx.arcTo(x + w, y, x + w, y + h, r);
	ctx.arcTo(x + w, y + h, x, y + h, r);
	ctx.arcTo(x, y + h, x, y, r);
	ctx.arcTo(x, y, x + w, y, r);
	ctx.closePath();
	//ctx.stroke();
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
	width: calc(100% - 4px - 1px);
	border: solid gray 1px;
	padding: 2px;
	border-radius: 2px;
}

.input-holder {
	display: flex;
}

.texture-options {
	background: #e0e0e0;
	border-radius: 3px;
	margin-bottom: 1em !important;
	padding: 1em;
}
</style>
