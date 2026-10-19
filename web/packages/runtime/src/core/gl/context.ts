// GPU resources shared by every evaluation: render-target format selection,
// a texture pool, the fullscreen quad and a framebuffer.

export type RenderTargetFormat = "rgba32f" | "rgba16f" | "rgba8";

const FORMATS: Record<RenderTargetFormat, { internal: number; type: number }> = {
    rgba32f: { internal: 0x8814 /* RGBA32F */, type: 0x1406 /* FLOAT */ },
    rgba16f: { internal: 0x881a /* RGBA16F */, type: 0x140b /* HALF_FLOAT */ },
    rgba8: { internal: 0x8058 /* RGBA8 */, type: 0x1401 /* UNSIGNED_BYTE */ },
};

export interface PooledTexture {
    texture: WebGLTexture;
    width: number;
    height: number;
}

export class GlContext {
    readonly gl: WebGL2RenderingContext;
    readonly format: RenderTargetFormat;
    readonly maxTextureSize: number;
    readonly framebuffer: WebGLFramebuffer;
    /** 1x1 opaque black, bound to unconnected inputs (what GL returns for an unbound sampler). */
    readonly blackTexture: WebGLTexture;

    private readonly vao: WebGLVertexArrayObject;
    private readonly free: PooledTexture[] = [];

    constructor(gl: WebGL2RenderingContext) {
        this.gl = gl;
        this.maxTextureSize = gl.getParameter(gl.MAX_TEXTURE_SIZE) as number;

        // Desktop renders into RGBA32F. Multi-pass nodes also sample their
        // intermediates with LINEAR filtering (and Fast AO builds mipmaps),
        // which RGBA32F only supports with OES_texture_float_linear; RGBA16F
        // is always filterable. Fall back further when the GPU can't render
        // to float targets at all (some mobile GPUs).
        const colorBufferFloat = !!gl.getExtension("EXT_color_buffer_float");
        const floatLinear = !!gl.getExtension("OES_texture_float_linear");
        if (colorBufferFloat && floatLinear) this.format = "rgba32f";
        else if (colorBufferFloat || gl.getExtension("EXT_color_buffer_half_float")) this.format = "rgba16f";
        else this.format = "rgba8";

        this.framebuffer = gl.createFramebuffer()!;

        this.blackTexture = gl.createTexture()!;
        gl.bindTexture(gl.TEXTURE_2D, this.blackTexture);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA8, 1, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array([0, 0, 0, 255]));
        setSampling(gl, gl.NEAREST, gl.REPEAT);

        // Same two triangles as the desktop quad: pos.xyz, uv.xy.
        // prettier-ignore
        const data = new Float32Array([
            -1, -1, 0,  0, 0,
             1, -1, 0,  1, 0,
            -1,  1, 0,  0, 1,
            -1,  1, 0,  0, 1,
             1, -1, 0,  1, 0,
             1,  1, 0,  1, 1,
        ]);
        this.vao = gl.createVertexArray()!;
        gl.bindVertexArray(this.vao);
        const vbo = gl.createBuffer()!;
        gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
        gl.bufferData(gl.ARRAY_BUFFER, data, gl.STATIC_DRAW);
        gl.enableVertexAttribArray(0);
        gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 20, 0);
        gl.enableVertexAttribArray(2);
        gl.vertexAttribPointer(2, 2, gl.FLOAT, false, 20, 12);
        gl.bindVertexArray(null);
    }

    /**
     * A render-target texture. Node textures sample NEAREST + REPEAT, like
     * the desktop node FBO textures.
     */
    acquire(width: number, height: number): PooledTexture {
        const gl = this.gl;
        const idx = this.free.findIndex((t) => t.width === width && t.height === height);
        if (idx >= 0) {
            // Reset sampling: CPU nodes leave their output LINEAR.
            const tex = this.free.splice(idx, 1)[0]!;
            gl.bindTexture(gl.TEXTURE_2D, tex.texture);
            setSampling(gl, gl.NEAREST, gl.REPEAT);
            return tex;
        }

        const texture = gl.createTexture()!;
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.texStorage2D(gl.TEXTURE_2D, 1, FORMATS[this.format].internal, width, height);
        setSampling(gl, gl.NEAREST, gl.REPEAT);
        return { texture, width, height };
    }

    release(tex: PooledTexture): void {
        this.free.push(tex);
    }

    /** Delete pooled textures that aren't in use. */
    trim(): void {
        for (const t of this.free) this.gl.deleteTexture(t.texture);
        this.free.length = 0;
    }

    /** Internal format + upload type of render targets in this context. */
    get formatInfo(): { internal: number; type: number } {
        return FORMATS[this.format];
    }

    bindTarget(tex: PooledTexture): void {
        const gl = this.gl;
        gl.bindFramebuffer(gl.FRAMEBUFFER, this.framebuffer);
        gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex.texture, 0);
        gl.viewport(0, 0, tex.width, tex.height);
    }

    drawQuad(): void {
        const gl = this.gl;
        gl.bindVertexArray(this.vao);
        gl.drawArrays(gl.TRIANGLES, 0, 6);
        gl.bindVertexArray(null);
    }

    dispose(): void {
        const gl = this.gl;
        this.trim();
        gl.deleteFramebuffer(this.framebuffer);
        gl.deleteTexture(this.blackTexture);
        gl.deleteVertexArray(this.vao);
    }
}

export function setSampling(gl: WebGL2RenderingContext, filter: number, wrap: number): void {
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, filter);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, filter);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, wrap);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, wrap);
}
