import { NodeRenderContext } from '@/lib/designer';
import { FloatProperty } from '@/lib/designer/properties';
import { DesignerNode } from "../../designer/designernode";

// https://xjavascript.com/view/639466/read-pixels-in-webgltexture-rendering-webgl-to-texture
export class Bevel extends DesignerNode {
    f:Float64Array;
    z:Float64Array;
    v:Uint16Array;
    grid:Float64Array;
    gridInner:Float64Array;
    gridOuter:Float64Array;

    // working pixels
    pixels:Uint8Array;

    readFbo:WebGLFramebuffer;

    distanceProp:FloatProperty;

	public init() {
		this.title = "Bevel";

		this.addInput("image");

		this.distanceProp = this.addFloatProperty("distance", "Distance", 50.0, 0.0, 100.0, 0.01);
        
        const width = this.designer.width;
        const height = this.designer.height;

        let size = Math.max(width, height);

        // make it span 3 textures wide to get wrapping
        this.f = new Float64Array(size * 3);
        this.z = new Float64Array(size * 3 + 1);
        this.v = new Uint16Array(size * 3);

        // convert image to float64 array
        let gridSize = width * height;
        this.grid = new Float64Array(gridSize);
        this.gridOuter = new Float64Array(gridSize);
        this.gridInner = new Float64Array(gridSize);
        this.pixels = new Uint8Array(width * height * 4);

        // create framebuffer for reading pixels from input texture
        this.readFbo = this.gl.createFramebuffer();

    }
    
    public render(context:NodeRenderContext) {
        const inputs = context.inputs;

        if (inputs.length == 0)
            return;

        let inputTexture = inputs[0].node.tex;

        const width = this.designer.width;
        const height = this.designer.height;
        const gridOuter = this.gridOuter;
        const gridInner = this.gridInner;
        const grid = this.grid;

        let gridSize = width * height;
        let gl = context.gl;
        var pixels = new Uint8Array(width * height * 4);

        gl.bindFramebuffer(gl.FRAMEBUFFER, this.readFbo);
        gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, inputTexture, 0);
        if (gl.checkFramebufferStatus(gl.FRAMEBUFFER) == gl.FRAMEBUFFER_COMPLETE) {
            
            gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
        } else {
            alert("Bevel: unable to read from FBO");
        }
        gl.bindFramebuffer(gl.FRAMEBUFFER, null);

        // copy data over from canvas data to grid
        for(let i = 0; i< gridSize;i++) {
            let col = 0;
            col += 255 - pixels[i * 4 + 0];
            col += 255 - pixels[i * 4 + 1];
            col += 255 - pixels[i * 4 + 2];


            //grid[i] = (col * 0.3333) / 255;
            let a = (col * 0.3333) / 255;
            //let a = 1.0 - (col * 0.3333) / 255; // invert
            this.gridOuter[i] = a === 1 ? 0 : a === 0 ? INF : Math.pow(Math.max(0, 0.5 - a), 2);
            this.gridInner[i] = a === 1 ? INF : a === 0 ? 0 : Math.pow(Math.max(0, a - 0.5), 2);
        }

        edt(this.gridOuter, width, height, this.f, this.v, this.z);
        edt(this.gridInner, width, height, this.f, this.v, this.z);

        let min = 255;
        let max = 0;
        let radius = this.distanceProp.getValue();
        let offset = 0.25;

        for(let i = 0; i< gridSize;i++) {
            // let col = grid[i];
            let d = Math.sqrt(gridOuter[i]) - Math.sqrt(gridInner[i]);
            //var col = 255 - 255 * (d / 50 + 0.25);
            var col = 255 - 255 * (d / radius + offset);
            // clamp
            col  = Math.max(0, Math.min(255, col));

            min = Math.min(min, col);
            max = Math.max(max, col);
            grid[i] = col;
        }

        let range = max - min;
        let scale = 255.0/range;
        console.log(min, max, range, scale);

        for(let i = 0; i< gridSize;i++) {
            //let col = (grid[i] - min) * scale;
            let col = 255 - (grid[i] - min) * scale; // de-invert
            
            pixels[i * 4 + 0] = col;
            pixels[i * 4 + 1] = col;
            pixels[i * 4 + 2] = col;
            pixels[i * 4 + 3] = 255;
        }

        gl.bindTexture(gl.TEXTURE_2D, this.tex);

		const level = 0;
		const internalFormat = gl.RGBA;
		const border = 0;
		const format = gl.RGBA;
		const type = gl.UNSIGNED_BYTE;
		const data = pixels;
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

// todo: find better algorithm
// https://github.com/parmanoir/Meijster-distance
// https://github.com/scijs/distance-transform

// https://github.com/mapbox/tiny-sdf/blob/master/index.js#L60
// https://observablehq.com/@mourner/fast-distance-transform

var INF = 1e20;

// 2D Euclidean squared distance transform by Felzenszwalb & Huttenlocher https://cs.brown.edu/~pff/papers/dt-final.pdf
function edt(data:Float64Array, width:number, height:number, f:Float64Array, v:Uint16Array, z:Float64Array) {
    for (let x = 0; x < width; x++) edt1d(data, x, width, height, f, v, z);
    for (let y = 0; y < height; y++) edt1d(data, y * width, 1, width, f, v, z);
}

// 1D squared distance transform
function edt1d(grid:Float64Array, offset:number, stride:number, length:number, f:Float64Array, v:Uint16Array, z:Float64Array) {
    let q, k, s, r;
    v[0] = 0;
    z[0] = -INF;
    z[1] = INF;

    // load line in array for convenient access
    // do it three times
    for (q = 0; q < length; q++) f[q] = grid[offset + q * stride];
    for (q = 0; q < length; q++) f[q + length] = grid[offset + q * stride];
    for (q = 0; q < length; q++) f[q + length + length] = grid[offset + q * stride];

    for (q = 1, k = 0, s = 0; q < length * 3; q++) {
        do {
            r = v[k];
            s = (f[q] - f[r] + q * q - r * r) / (q - r) / 2;
        // todo: wrap k in z[k] and make --k > -length
        } while (s <= z[k] && --k > -1); 

        k++;
        v[k] = q;
        z[k] = s;
        z[k + 1] = INF;
    }

    // only cope over mid section
    // |----|----|----|
    //         ^
    for (q = length, k = 0; q < length + length; q++) {
        while (z[k + 1] < q) k++;
        r = v[k];
        grid[offset + (q - length) * stride] = f[r] + (q - r) * (q - r);
    }
}
