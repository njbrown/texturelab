// Shader assembly (mirrors TextureRenderer::buildShaderForNode) and
// compilation with KHR_parallel_shader_compile when available.

import type { TextureNode } from "../node.js";
import type { PropType } from "../props.js";
import { curveLib, fragmentPreamble, gradientLib, randomLib, standardVertexSource } from "./shaderlib.js";

const PROP_UNIFORM_TYPES: Partial<Record<PropType, string>> = {
    int: "int",
    float: "float",
    bool: "bool",
    enum: "int",
    color: "vec4",
    gradient: "Gradient",
    image: "sampler2D",
    curve: "Curve",
};

export interface AssembledShader {
    source: string;
    /** Line count before the node's own source, for mapping error lines. */
    prefixLines: number;
}

export function assembleFragmentShader(node: TextureNode): AssembledShader {
    let decls = "";
    for (const input of node.inputs) {
        decls += `uniform sampler2D ${input};\n`;
        decls += `uniform bool ${input}_connected;\n`;
    }
    for (const prop of node.props.values()) {
        const type = PROP_UNIFORM_TYPES[prop.type];
        if (type) decls += `uniform ${type} prop_${prop.name};\n`;
    }
    decls += "\n";

    const prefix = fragmentPreamble + randomLib + gradientLib + curveLib + decls;
    return {
        source: prefix + node.shaderSource,
        prefixLines: prefix.split("\n").length - 1,
    };
}

export interface Program {
    program: WebGLProgram;
    uniforms: Map<string, WebGLUniformLocation | null>;
}

export class ShaderCompileError extends Error {
    override name = "ShaderCompileError";
}

const PARALLEL_COMPLETION_STATUS = 0x91b1; // COMPLETION_STATUS_KHR

interface Pending {
    vs: WebGLShader;
    fs: WebGLShader;
    program: WebGLProgram;
    shader: AssembledShader;
}

/** Compiles and caches node programs, keyed by full fragment source. */
export class ProgramCache {
    private readonly gl: WebGL2RenderingContext;
    private readonly parallel: boolean;
    private readonly ready = new Map<string, Program | ShaderCompileError>();
    private readonly pending = new Map<string, Pending>();

    constructor(gl: WebGL2RenderingContext) {
        this.gl = gl;
        this.parallel = !!gl.getExtension("KHR_parallel_shader_compile");
    }

    /** Start compiling; doesn't block. */
    request(shader: AssembledShader): void {
        if (this.ready.has(shader.source) || this.pending.has(shader.source)) return;
        const gl = this.gl;
        const vs = gl.createShader(gl.VERTEX_SHADER)!;
        gl.shaderSource(vs, standardVertexSource);
        gl.compileShader(vs);
        const fs = gl.createShader(gl.FRAGMENT_SHADER)!;
        gl.shaderSource(fs, shader.source);
        gl.compileShader(fs);
        const program = gl.createProgram()!;
        gl.attachShader(program, vs);
        gl.attachShader(program, fs);
        // Same attribute slots as the desktop quad (VertexUsage::Position/TexCoord0).
        gl.bindAttribLocation(program, 0, "a_pos");
        gl.bindAttribLocation(program, 2, "a_texCoord");
        gl.linkProgram(program);
        this.pending.set(shader.source, { vs, fs, program, shader });
    }

    /** Resolves once every requested program has finished compiling. */
    async settle(yieldFn: () => Promise<void>): Promise<void> {
        if (this.parallel) {
            while ([...this.pending.values()].some((p) => !this.gl.getProgramParameter(p.program, PARALLEL_COMPLETION_STATUS))) {
                await yieldFn();
            }
        }
        for (const [key, p] of this.pending) this.ready.set(key, this.finish(p));
        this.pending.clear();
    }

    /** Program for an assembled shader (compiling synchronously if needed). */
    get(shader: AssembledShader): Program | ShaderCompileError {
        let result = this.ready.get(shader.source);
        if (!result) {
            this.request(shader);
            const p = this.pending.get(shader.source)!;
            this.pending.delete(shader.source);
            result = this.finish(p);
            this.ready.set(shader.source, result);
        }
        return result;
    }

    private finish(p: Pending): Program | ShaderCompileError {
        const gl = this.gl;
        let error: ShaderCompileError | null = null;
        if (!gl.getProgramParameter(p.program, gl.LINK_STATUS)) {
            const log =
                gl.getShaderInfoLog(p.fs) || gl.getShaderInfoLog(p.vs) || gl.getProgramInfoLog(p.program) || "unknown error";
            error = new ShaderCompileError(remapErrorLines(log, p.shader.prefixLines));
        }
        gl.detachShader(p.program, p.vs);
        gl.detachShader(p.program, p.fs);
        gl.deleteShader(p.vs);
        gl.deleteShader(p.fs);
        if (error) {
            gl.deleteProgram(p.program);
            return error;
        }
        return { program: p.program, uniforms: new Map() };
    }

    dispose(): void {
        for (const p of this.ready.values()) if (!(p instanceof Error)) this.gl.deleteProgram(p.program);
        this.ready.clear();
    }
}

/** Rewrite "ERROR: 0:123:" line numbers to be relative to the node source. */
function remapErrorLines(log: string, prefixLines: number): string {
    return log.replace(/(ERROR|WARNING): (\d+):(\d+):/g, (_m, kind, file, line) => {
        const rel = Number(line) - prefixLines;
        return rel > 0 ? `${kind}: node source line ${rel}:` : `${kind}: ${file}:${line} (prelude):`;
    });
}

export function uniformLocation(gl: WebGL2RenderingContext, p: Program, name: string): WebGLUniformLocation | null {
    let loc = p.uniforms.get(name);
    if (loc === undefined) {
        loc = gl.getUniformLocation(p.program, name);
        p.uniforms.set(name, loc);
    }
    return loc;
}
