// Custom-renderer infrastructure, mirroring src/texturelab/graphics/
// noderenderer.h: RenderResourceCache, NodeRenderContext, ScopedTextureParams
// and NodeTextureRenderer. Ported multi-pass nodes (libraries/v3/*.ts) are
// written against this API so they read line-for-line like their C++ twins.

import type { GlContext } from "./context.js";
import { ShaderCompileError } from "./program.js";
import { standardVertexSource } from "./shaderlib.js";

export { standardVertexSource };

const ES_HEADER = `#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;
`;

/**
 * Standalone renderer shaders are written for desktop GLSL (`#version 150`).
 * Swap that line for the GLSL ES 3.00 header; the bodies are unchanged.
 */
export function toGlslEs(source: string): string {
    const replaced = source.replace(/^\s*#version[^\n]*\n/, ES_HEADER);
    return replaced === source ? ES_HEADER + source : replaced;
}

// Input texture binding passed to custom renderers.
export interface NodeInputBinding {
    name: string;
    texture: WebGLTexture;
}

// Mirrors RenderResourceCache: intermediate textures and compiled shaders.
export class RenderResourceCache {
    private readonly gl: WebGL2RenderingContext;
    private readonly ctx: GlContext;
    private readonly pool: { texture: WebGLTexture; width: number; height: number; inUse: boolean }[] = [];
    private readonly shaderCache = new Map<string, WebGLProgram | ShaderCompileError>();

    constructor(ctx: GlContext) {
        this.ctx = ctx;
        this.gl = ctx.gl;
    }

    // NEAREST + CLAMP_TO_EDGE, the defaults every pooled texture carries.
    static applyDefaultTextureParams(gl: WebGL2RenderingContext, texture: WebGLTexture): void {
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.bindTexture(gl.TEXTURE_2D, null);
    }

    /**
     * Pooled intermediate texture, reset to the default parameters. Storage is
     * mutable (texImage2D) so renderers can glGenerateMipmap on it, as Fast AO
     * does.
     */
    acquireTexture(width: number, height: number): WebGLTexture {
        const gl = this.gl;
        const free = this.pool.find((t) => !t.inUse && t.width === width && t.height === height);
        if (free) {
            free.inUse = true;
            RenderResourceCache.applyDefaultTextureParams(gl, free.texture);
            return free.texture;
        }

        const texture = gl.createTexture()!;
        gl.bindTexture(gl.TEXTURE_2D, texture);
        const f = this.ctx.formatInfo;
        gl.texImage2D(gl.TEXTURE_2D, 0, f.internal, width, height, 0, gl.RGBA, f.type, null);
        RenderResourceCache.applyDefaultTextureParams(gl, texture);
        this.pool.push({ texture, width, height, inUse: true });
        return texture;
    }

    releaseTexture(texture: WebGLTexture): void {
        const entry = this.pool.find((t) => t.texture === texture);
        if (entry) entry.inUse = false;
    }

    /** Compile (once per key) a standalone program. Throws ShaderCompileError on failure. */
    getOrCompileShader(key: string, vertexSource: string, fragmentSource: string): WebGLProgram {
        let cached = this.shaderCache.get(key);
        if (!cached) {
            cached = this.compile(key, vertexSource, toGlslEs(fragmentSource));
            this.shaderCache.set(key, cached);
        }
        if (cached instanceof ShaderCompileError) throw cached;
        return cached;
    }

    bindFboToTexture(texture: WebGLTexture): void {
        const gl = this.gl;
        gl.bindFramebuffer(gl.FRAMEBUFFER, this.ctx.framebuffer);
        gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture, 0);
    }

    /**
     * Read a texture back as float RGBA (rows bottom-to-top), like the C++
     * CPU nodes' glReadPixels(GL_RGBA, GL_FLOAT).
     */
    readPixelsFloat(texture: WebGLTexture, width: number, height: number): Float32Array {
        const gl = this.gl;
        this.bindFboToTexture(texture);
        const out = new Float32Array(width * height * 4);
        if (this.ctx.format === "rgba8") {
            const bytes = new Uint8Array(width * height * 4);
            gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, bytes);
            for (let i = 0; i < bytes.length; i++) out[i] = bytes[i]! / 255;
        } else {
            gl.readPixels(0, 0, width, height, gl.RGBA, gl.FLOAT, out);
        }
        return out;
    }

    /** Upload float RGBA into an existing texture (the CPU nodes' glTexImage2D). */
    uploadPixelsFloat(texture: WebGLTexture, width: number, height: number, data: Float32Array): void {
        const gl = this.gl;
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, false);
        gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
        if (this.ctx.format === "rgba8") {
            const bytes = new Uint8Array(data.length);
            for (let i = 0; i < data.length; i++) bytes[i] = Math.round(Math.min(1, Math.max(0, data[i]!)) * 255);
            gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, bytes);
        } else {
            gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, width, height, gl.RGBA, gl.FLOAT, data);
        }
        gl.bindTexture(gl.TEXTURE_2D, null);
    }

    dispose(): void {
        const gl = this.gl;
        for (const t of this.pool) gl.deleteTexture(t.texture);
        this.pool.length = 0;
        for (const p of this.shaderCache.values()) if (!(p instanceof ShaderCompileError)) gl.deleteProgram(p);
        this.shaderCache.clear();
    }

    private compile(key: string, vertexSource: string, fragmentSource: string): WebGLProgram | ShaderCompileError {
        const gl = this.gl;
        const vs = gl.createShader(gl.VERTEX_SHADER)!;
        gl.shaderSource(vs, vertexSource);
        gl.compileShader(vs);
        const fs = gl.createShader(gl.FRAGMENT_SHADER)!;
        gl.shaderSource(fs, fragmentSource);
        gl.compileShader(fs);
        const program = gl.createProgram()!;
        gl.attachShader(program, vs);
        gl.attachShader(program, fs);
        gl.bindAttribLocation(program, 0, "a_pos");
        gl.bindAttribLocation(program, 2, "a_texCoord");
        gl.linkProgram(program);
        const ok = gl.getProgramParameter(program, gl.LINK_STATUS) as boolean;
        const log = ok ? "" : gl.getShaderInfoLog(fs) || gl.getShaderInfoLog(vs) || gl.getProgramInfoLog(program) || "";
        gl.deleteShader(vs);
        gl.deleteShader(fs);
        if (!ok) {
            gl.deleteProgram(program);
            return new ShaderCompileError(`shader "${key}": ${log}`);
        }
        return program;
    }
}

// Mirrors NodeRenderContext: a view into the GL state passed to renderers.
export class NodeRenderContext {
    readonly gl: WebGL2RenderingContext;
    readonly cache: RenderResourceCache;
    readonly outputTexture: WebGLTexture;
    readonly textureWidth: number;
    readonly textureHeight: number;
    readonly randomSeed: number;
    /** Connected inputs, in the node's declaration order. */
    readonly inputs: NodeInputBinding[];
    private readonly glContext: GlContext;

    constructor(
        glContext: GlContext,
        cache: RenderResourceCache,
        outputTexture: WebGLTexture,
        width: number,
        height: number,
        randomSeed: number,
        inputs: NodeInputBinding[],
    ) {
        this.glContext = glContext;
        this.gl = glContext.gl;
        this.cache = cache;
        this.outputTexture = outputTexture;
        this.textureWidth = width;
        this.textureHeight = height;
        this.randomSeed = randomSeed;
        this.inputs = inputs;
    }

    // Texture bound to the named input, or null if it isn't connected.
    inputTexture(name: string): WebGLTexture | null {
        return this.inputs.find((i) => i.name === name)?.texture ?? null;
    }

    // Bind shader, set viewport, set _textureSize and _seed uniforms.
    useShader(program: WebGLProgram): void {
        const gl = this.gl;
        gl.useProgram(program);
        gl.viewport(0, 0, this.textureWidth, this.textureHeight);
        gl.uniform2f(gl.getUniformLocation(program, "_textureSize"), this.textureWidth, this.textureHeight);
        gl.uniform1f(gl.getUniformLocation(program, "_seed"), this.randomSeed);
    }

    // Bind a texture to a sampler uniform.
    bindTexture(program: WebGLProgram, uniformName: string, texture: WebGLTexture, unit: number): void {
        const gl = this.gl;
        gl.activeTexture(gl.TEXTURE0 + unit);
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.uniform1i(gl.getUniformLocation(program, uniformName), unit);
    }

    // Draw a fullscreen quad using the currently bound shader.
    drawQuad(): void {
        this.glContext.drawQuad();
    }

    /** Fill the output with a solid color (the renderers' "no input" path). */
    clearOutput(r: number, g: number, b: number, a: number): void {
        const gl = this.gl;
        this.cache.bindFboToTexture(this.outputTexture);
        gl.viewport(0, 0, this.textureWidth, this.textureHeight);
        gl.clearColor(r, g, b, a);
        gl.clear(gl.COLOR_BUFFER_BIT);
    }
}

/**
 * Mirrors ScopedTextureParams: temporarily re-parameterise a texture for the
 * duration of `body`. Unlike the C++ guard, which resets to the pool defaults
 * (NEAREST + CLAMP_TO_EDGE), this restores whatever the texture had before —
 * node output textures are REPEAT, and borrowing one as an input shouldn't
 * change how later nodes sample it.
 */
export function withTextureParams<T>(
    gl: WebGL2RenderingContext,
    textures: { texture: WebGLTexture; minFilter: number; magFilter: number; wrap?: number }[],
    body: () => T,
): T {
    const saved = textures.map(({ texture }) => {
        gl.bindTexture(gl.TEXTURE_2D, texture);
        return {
            texture,
            min: gl.getTexParameter(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER) as number,
            mag: gl.getTexParameter(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER) as number,
            s: gl.getTexParameter(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S) as number,
            t: gl.getTexParameter(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T) as number,
        };
    });
    for (const { texture, minFilter, magFilter, wrap } of textures) {
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, minFilter);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, magFilter);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, wrap ?? gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, wrap ?? gl.CLAMP_TO_EDGE);
    }
    gl.bindTexture(gl.TEXTURE_2D, null);
    try {
        return body();
    } finally {
        for (const s of saved) {
            gl.bindTexture(gl.TEXTURE_2D, s.texture);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, s.min);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, s.mag);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, s.s);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, s.t);
        }
        gl.bindTexture(gl.TEXTURE_2D, null);
    }
}

// Mirrors NodeTextureRenderer. Must write its result to ctx.outputTexture.
export interface NodeTextureRenderer<D = unknown> {
    render(ctx: NodeRenderContext, data: D): void;
}
