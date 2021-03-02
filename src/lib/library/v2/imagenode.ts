import { GpuDesignerNode } from "../../designer/gpudesignernode";
import { Color } from "@/lib/designer/color";
import { Image } from "@/lib/designer/image";
import { DesignerNode } from "@/lib/designer/designernode";
import { NodeRenderContext } from "@/lib/designer";
import { ImageProperty } from "@/lib/designer/properties";

export class ImageNode extends DesignerNode {
	canvas:HTMLCanvasElement;
	imageProp: ImageProperty;
	public init() {
		this.title = "Image";

		this.canvas = document.createElement("canvas") as HTMLCanvasElement;
		this.canvas.width = this.designer.width;
		this.canvas.height = this.designer.height;

		let ctx = this.canvas.getContext("2d");
		ctx.fillStyle = "rgb(0,255,0)";
		ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

		this.imageProp = this.addImageProperty("image", "Image", Image.empty());
	}

    public render(context: NodeRenderContext) {
		let gl = context.gl as WebGL2RenderingContext;

		this.canvas.width = this.designer.width;
		this.canvas.height = this.designer.height;

		let ctx = this.canvas.getContext("2d");
		ctx.fillStyle = "rgb(0,255,0)";
		ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
		if (this.imageProp.value && !this.imageProp.value.isEmpty)
			ctx.drawImage(this.imageProp.value.canvas, 0, 0, this.canvas.width, this.canvas.height);

		//todo: render image to canvas
		// stretch image if must

		gl.bindTexture(gl.TEXTURE_2D, this.tex);

		const level = 0;
		const internalFormat = gl.RGBA;
		const border = 0;
		const format = gl.RGBA;
		const type = gl.UNSIGNED_BYTE;
		const data = this.canvas;
		gl.texImage2D(
			gl.TEXTURE_2D,
			level,
			internalFormat,
			this.designer.width,
			this.designer.height,
			border,
			format,
			type,
			data
		);

		// set the filtering so we don't need mips
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);

		gl.bindTexture(gl.TEXTURE_2D, null);
    }
}
