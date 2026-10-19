// Graph evaluation. Mirrors TextureRenderer/RenderWorker's single-pass path
// (src/texturelab/graphics/): one fullscreen draw per node, inputs bound as
// samplers, props as prop_<name> uniforms, _seed and _textureSize.
//
// Differences from desktop, by design:
// - Only nodes that feed a texture channel are rendered.
// - Intermediate textures are released as soon as their last consumer has
//   rendered, so peak GPU memory follows graph width instead of node count.

import { TEXTURE_CHANNELS, type TextureChannel, type TextureDocument } from "./document.js";
import { GlContext, setSampling, type PooledTexture, type RenderTargetFormat } from "./gl/context.js";
import { NodeRenderContext, RenderResourceCache, type NodeInputBinding } from "./gl/noderenderer.js";
import { assembleFragmentShader, ProgramCache, ShaderCompileError, uniformLocation, type Program } from "./gl/program.js";
import type { TextureNode } from "./node.js";
import type { ImageProp } from "./props.js";

export interface RuntimeOptions {
    /**
     * Canvas to create the WebGL2 context on. Defaults to an internal
     * OffscreenCanvas (or a detached <canvas> where that's unavailable).
     */
    canvas?: HTMLCanvasElement | OffscreenCanvas;
    /** Upper bound on evaluation resolution. Default 2048. */
    maxResolution?: number;
}

export interface EvaluateOptions {
    /** Render at this resolution instead of the file's own (still capped by maxResolution). */
    resolution?: number;
    onProgress?: (progress: EvaluateProgress) => void;
    signal?: AbortSignal;
}

export interface EvaluateProgress {
    done: number;
    total: number;
    /** typeName of the node just rendered. */
    node?: string;
}

export type WarningKind =
    | "unknown-node"
    | "unsupported-node"
    | "shader-error"
    | "render-error"
    | "missing-channel"
    | "image-decode";

export interface EvaluationWarning {
    kind: WarningKind;
    message: string;
    nodeId?: string;
    typeName?: string;
}

export interface ChannelPixels<T extends Uint8Array | Float32Array> {
    width: number;
    height: number;
    /** RGBA, rows bottom-to-top (GL order: row 0 is uv.y = 0). */
    data: T;
}

export interface TextureMaps {
    albedo?: ChannelPixels<Uint8Array>;
    normal?: ChannelPixels<Uint8Array>;
    metalness?: ChannelPixels<Uint8Array>;
    roughness?: ChannelPixels<Uint8Array>;
    ao?: ChannelPixels<Uint8Array>;
    alpha?: ChannelPixels<Uint8Array>;
    /** Float, so displacement doesn't band. */
    height?: ChannelPixels<Float32Array>;
}

export class EvaluationResult {
    readonly width: number;
    readonly height: number;
    readonly warnings: EvaluationWarning[];
    readonly timeMs: number;
    readonly nodesRendered: number;
    private readonly runtime: TextureRuntime;
    private textures: Map<TextureChannel, PooledTexture>;

    /** @internal */
    constructor(
        runtime: TextureRuntime,
        width: number,
        height: number,
        textures: Map<TextureChannel, PooledTexture>,
        warnings: EvaluationWarning[],
        timeMs: number,
        nodesRendered: number,
    ) {
        this.runtime = runtime;
        this.width = width;
        this.height = height;
        this.textures = textures;
        this.warnings = warnings;
        this.timeMs = timeMs;
        this.nodesRendered = nodesRendered;
    }

    /** Channels this result has a texture for. */
    get channels(): TextureChannel[] {
        return [...this.textures.keys()];
    }

    readUint8(channel: TextureChannel): ChannelPixels<Uint8Array> | null {
        const tex = this.textures.get(channel);
        return tex ? this.runtime.readUint8(tex) : null;
    }

    readFloat32(channel: TextureChannel): ChannelPixels<Float32Array> | null {
        const tex = this.textures.get(channel);
        return tex ? this.runtime.readFloat32(tex) : null;
    }

    /** Read back every channel in the form the viewer wants. */
    toMaps(): TextureMaps {
        const maps: TextureMaps = {};
        for (const channel of this.textures.keys()) {
            if (channel === "height") maps.height = this.readFloat32("height")!;
            else maps[channel] = this.readUint8(channel)!;
        }
        return maps;
    }

    /** Free the GPU textures. The result can't be read afterwards. */
    dispose(): void {
        const released = new Set<PooledTexture>();
        for (const tex of this.textures.values()) {
            if (released.has(tex)) continue;
            released.add(tex);
            this.runtime.releaseTexture(tex);
        }
        this.textures = new Map();
    }
}

const nextFrame = (): Promise<void> =>
    new Promise((resolve) => {
        if (typeof requestAnimationFrame === "function") requestAnimationFrame(() => resolve());
        else setTimeout(resolve, 0);
    });

export class TextureRuntime {
    readonly gl: WebGL2RenderingContext;
    private readonly ctx: GlContext;
    private readonly programs: ProgramCache;
    private readonly cache: RenderResourceCache;
    private readonly maxResolution: number;
    private readonly imageCache = new Map<string, Promise<WebGLTexture | null>>();
    private copyProgram: Program | null = null;
    private uint8Target: { tex: PooledTexture; texture8: WebGLTexture } | null = null;

    constructor(options: RuntimeOptions = {}) {
        const canvas =
            options.canvas ??
            (typeof OffscreenCanvas !== "undefined" ? new OffscreenCanvas(1, 1) : document.createElement("canvas"));
        const gl = canvas.getContext("webgl2", {
            alpha: true,
            antialias: false,
            depth: false,
            stencil: false,
            premultipliedAlpha: false,
            preserveDrawingBuffer: false,
        }) as WebGL2RenderingContext | null;
        if (!gl) throw new Error("WebGL2 is not available");
        this.gl = gl;
        this.ctx = new GlContext(gl);
        this.programs = new ProgramCache(gl);
        this.cache = new RenderResourceCache(this.ctx);
        this.maxResolution = Math.min(options.maxResolution ?? 2048, this.ctx.maxTextureSize);
    }

    /** Precision of intermediate render targets on this GPU. */
    get renderTargetFormat(): RenderTargetFormat {
        return this.ctx.format;
    }

    async evaluate(doc: TextureDocument, options: EvaluateOptions = {}): Promise<EvaluationResult> {
        const start = performance.now();
        const gl = this.gl;
        const warnings: EvaluationWarning[] = [];

        for (const typeName of new Set(doc.skippedTypes)) {
            warnings.push({
                kind: "unknown-node",
                typeName,
                message: `Node type "${typeName}" isn't in the ${doc.libraryVersion} library; it was skipped`,
            });
        }

        const size = clampResolution(options.resolution ?? doc.width, this.maxResolution);

        // inputs: rightNodeId -> inputName -> leftNodeId
        const inputsOf = new Map<string, Map<string, string>>();
        for (const c of doc.connections) {
            let m = inputsOf.get(c.rightNodeId);
            if (!m) inputsOf.set(c.rightNodeId, (m = new Map()));
            m.set(c.rightNodeInput, c.leftNodeId);
        }

        // Channel targets.
        const targets = new Map<TextureChannel, string>();
        for (const channel of TEXTURE_CHANNELS) {
            const id = doc.textureChannels[channel];
            if (!id) continue;
            if (doc.nodes.has(id)) targets.set(channel, id);
            else warnings.push({ kind: "missing-channel", message: `The ${channel} channel points at a node that didn't load` });
        }

        // Topological order of everything the targets depend on (DFS post-order).
        const order: TextureNode[] = [];
        const visited = new Set<string>();
        const visit = (id: string) => {
            if (visited.has(id)) return;
            visited.add(id);
            for (const left of inputsOf.get(id)?.values() ?? []) visit(left);
            order.push(doc.nodes.get(id)!);
        };
        for (const id of targets.values()) visit(id);

        // How many rendered nodes consume each node's output, so intermediates
        // can be released after their last use.
        const remainingUses = new Map<string, number>();
        for (const node of order) {
            for (const left of inputsOf.get(node.id)?.values() ?? []) {
                remainingUses.set(left, (remainingUses.get(left) ?? 0) + 1);
            }
        }
        const targetIds = new Set(targets.values());

        // Compile every shader up front so the driver can work in parallel,
        // and decode embedded images meanwhile.
        const imageTextures = new Map<string, WebGLTexture | null>();
        const imageLoads: Promise<void>[] = [];
        for (const node of order) {
            const reason = node.unsupportedReason;
            if (reason) {
                warnings.push({
                    kind: "unsupported-node",
                    nodeId: node.id,
                    typeName: node.typeName,
                    message: `"${node.title}" (${node.typeName}): ${reason}; it renders black`,
                });
                continue;
            }
            if (!usesCustomRenderer(node)) this.programs.request(assembleFragmentShader(node));
            for (const prop of node.props.values()) {
                if (prop.type !== "image" || !(prop as ImageProp).value) continue;
                const url = (prop as ImageProp).value;
                imageLoads.push(
                    this.loadImage(url).then((tex) => {
                        imageTextures.set(url, tex);
                        if (!tex) {
                            warnings.push({
                                kind: "image-decode",
                                nodeId: node.id,
                                typeName: node.typeName,
                                message: `"${node.title}": couldn't decode embedded image`,
                            });
                        }
                    }),
                );
            }
        }
        await Promise.all([this.programs.settle(nextFrame), ...imageLoads]);

        const outputs = new Map<string, PooledTexture>();
        let lastYield = performance.now();
        let done = 0;

        try {
            for (const node of order) {
                options.signal?.throwIfAborted();

                const target = this.ctx.acquire(size, size);
                outputs.set(node.id, target);
                this.ctx.bindTarget(target);

                if (node.unsupportedReason) {
                    gl.clearColor(0, 0, 0, 1);
                    gl.clear(gl.COLOR_BUFFER_BIT);
                } else if (usesCustomRenderer(node)) {
                    const error = this.renderCustom(node, target, doc, size, inputsOf.get(node.id), outputs);
                    if (error) warnings.push(error);
                } else {
                    const program = this.programs.get(assembleFragmentShader(node));
                    if (program instanceof ShaderCompileError) {
                        warnings.push({
                            kind: "shader-error",
                            nodeId: node.id,
                            typeName: node.typeName,
                            message: `"${node.title}" (${node.typeName}) failed to compile; it renders black.\n${program.message}`,
                        });
                        gl.clearColor(0, 0, 0, 1);
                        gl.clear(gl.COLOR_BUFFER_BIT);
                    } else {
                        this.drawNode(node, program, doc, size, inputsOf.get(node.id), outputs, imageTextures);
                    }
                }

                // Release inputs nobody else needs.
                for (const left of inputsOf.get(node.id)?.values() ?? []) {
                    const uses = (remainingUses.get(left) ?? 1) - 1;
                    remainingUses.set(left, uses);
                    if (uses === 0 && !targetIds.has(left)) {
                        const tex = outputs.get(left);
                        if (tex) this.ctx.release(tex);
                        outputs.delete(left);
                    }
                }

                done++;
                options.onProgress?.({ done, total: order.length, node: node.typeName });

                // Keep the page responsive on big graphs.
                if (performance.now() - lastYield > 12) {
                    await nextFrame();
                    lastYield = performance.now();
                }
            }
        } catch (e) {
            for (const tex of outputs.values()) this.ctx.release(tex);
            throw e;
        }

        gl.bindFramebuffer(gl.FRAMEBUFFER, null);

        const channelTextures = new Map<TextureChannel, PooledTexture>();
        for (const [channel, id] of targets) channelTextures.set(channel, outputs.get(id)!);

        return new EvaluationResult(this, size, size, channelTextures, warnings, performance.now() - start, done);
    }

    /**
     * Multi-pass (createRenderer) and CPU (cpuProcess) nodes. Returns a
     * warning if the node failed; its output is then black.
     */
    private renderCustom(
        node: TextureNode,
        target: PooledTexture,
        doc: TextureDocument,
        size: number,
        inputs: Map<string, string> | undefined,
        outputs: Map<string, PooledTexture>,
    ): EvaluationWarning | null {
        const gl = this.gl;
        // Connected inputs in declaration order.
        const bindings: NodeInputBinding[] = [];
        for (const name of node.inputs) {
            const leftId = inputs?.get(name);
            const tex = leftId ? outputs.get(leftId) : undefined;
            if (tex) bindings.push({ name, texture: tex.texture });
        }
        const ctx = new NodeRenderContext(
            this.ctx,
            this.cache,
            target.texture,
            size,
            size,
            doc.randomSeed + node.randomSeed,
            bindings,
        );

        try {
            const renderer = node.createRenderer?.();
            if (renderer) renderer.render(ctx, node.createRenderData?.());
            else node.cpuProcess!(ctx);
            return null;
        } catch (e) {
            ctx.clearOutput(0, 0, 0, 1);
            const isShader = e instanceof ShaderCompileError;
            return {
                kind: isShader ? "shader-error" : "render-error",
                nodeId: node.id,
                typeName: node.typeName,
                message: `"${node.title}" (${node.typeName}) failed${isShader ? " to compile" : ""}; it renders black.\n${(e as Error).message}`,
            };
        } finally {
            // Leave GL state as single-pass nodes expect it.
            gl.disable(gl.BLEND);
            gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        }
    }

    private drawNode(
        node: TextureNode,
        p: Program,
        doc: TextureDocument,
        size: number,
        inputs: Map<string, string> | undefined,
        outputs: Map<string, PooledTexture>,
        imageTextures: Map<string, WebGLTexture | null>,
    ): void {
        const gl = this.gl;
        gl.useProgram(p.program);
        const loc = (name: string) => uniformLocation(gl, p, name);

        // Inputs, one texture unit each in declaration order (like RenderWorker).
        let unit = 0;
        for (const inputName of node.inputs) {
            const leftId = inputs?.get(inputName);
            const tex = leftId ? outputs.get(leftId) : undefined;
            gl.activeTexture(gl.TEXTURE0 + unit);
            gl.bindTexture(gl.TEXTURE_2D, tex ? tex.texture : this.ctx.blackTexture);
            gl.uniform1i(loc(inputName), unit);
            gl.uniform1i(loc(`${inputName}_connected`), tex ? 1 : 0);
            unit++;
        }

        gl.uniform1f(loc("_seed"), doc.randomSeed + node.randomSeed);
        gl.uniform2f(loc("_textureSize"), size, size);

        for (const prop of node.props.values()) {
            const name = `prop_${prop.name}`;
            switch (prop.type) {
                case "int":
                    gl.uniform1i(loc(name), prop.value);
                    break;
                case "float":
                    gl.uniform1f(loc(name), prop.value);
                    break;
                case "bool":
                    gl.uniform1i(loc(name), prop.value ? 1 : 0);
                    break;
                case "enum":
                    gl.uniform1i(loc(name), prop.index);
                    break;
                case "color": {
                    const c = prop.value;
                    gl.uniform4f(loc(name), c.r, c.g, c.b, c.a);
                    break;
                }
                case "gradient": {
                    const points = prop.value.points;
                    gl.uniform1i(loc(`${name}.numPoints`), points.length);
                    points.forEach((pt, i) => {
                        gl.uniform3f(loc(`${name}.colors[${i}]`), pt.color.r, pt.color.g, pt.color.b);
                        gl.uniform1f(loc(`${name}.positions[${i}]`), pt.position);
                    });
                    break;
                }
                case "curve": {
                    const points = prop.value.points.slice(0, 32);
                    gl.uniform1i(loc(`${name}.numPoints`), points.length);
                    points.forEach((pt, i) => {
                        gl.uniform2f(loc(`${name}.anchors[${i}]`), pt.x, pt.y);
                        gl.uniform2f(loc(`${name}.handleR[${i}]`), pt.x + pt.rx, pt.y + pt.ry);
                        gl.uniform2f(loc(`${name}.handleL[${i}]`), pt.x + pt.lx, pt.y + pt.ly);
                    });
                    break;
                }
                case "image": {
                    const tex = prop.value ? imageTextures.get(prop.value) : null;
                    gl.activeTexture(gl.TEXTURE0 + unit);
                    gl.bindTexture(gl.TEXTURE_2D, tex ?? this.ctx.blackTexture);
                    gl.uniform1i(loc(name), unit);
                    unit++;
                    break;
                }
                case "string":
                    break;
            }
        }

        this.ctx.drawQuad();
    }

    /**
     * Decode an embedded image. Like ImageProp::updateTexture, the image is
     * flipped so its top row lands at uv.y = 1, and sampled NEAREST + REPEAT.
     */
    private loadImage(dataUrl: string): Promise<WebGLTexture | null> {
        let p = this.imageCache.get(dataUrl);
        if (!p) {
            p = (async () => {
                try {
                    const blob = await (await fetch(dataUrl)).blob();
                    const bitmap = await createImageBitmap(blob, { imageOrientation: "flipY", premultiplyAlpha: "none" });
                    const gl = this.gl;
                    const tex = gl.createTexture()!;
                    gl.bindTexture(gl.TEXTURE_2D, tex);
                    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, false);
                    gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
                    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA8, gl.RGBA, gl.UNSIGNED_BYTE, bitmap);
                    setSampling(gl, gl.NEAREST, gl.REPEAT);
                    bitmap.close();
                    return tex;
                } catch {
                    return null;
                }
            })();
            this.imageCache.set(dataUrl, p);
        }
        return p;
    }

    /** @internal */
    releaseTexture(tex: PooledTexture): void {
        this.ctx.release(tex);
    }

    /** @internal Read a node texture as RGBA8 (converted on the GPU). */
    readUint8(src: PooledTexture): ChannelPixels<Uint8Array> {
        const gl = this.gl;
        const { width, height } = src;

        if (!this.uint8Target || this.uint8Target.tex.width !== width || this.uint8Target.tex.height !== height) {
            if (this.uint8Target) gl.deleteTexture(this.uint8Target.texture8);
            const texture8 = gl.createTexture()!;
            gl.bindTexture(gl.TEXTURE_2D, texture8);
            gl.texStorage2D(gl.TEXTURE_2D, 1, gl.RGBA8, width, height);
            this.uint8Target = { tex: { texture: texture8, width, height }, texture8 };
        }

        // Draw src into the RGBA8 target; the fixed-point conversion rounds
        // like the desktop exporter's float->8-bit path.
        this.ctx.bindTarget(this.uint8Target.tex);
        const p = this.getCopyProgram();
        gl.useProgram(p.program);
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, src.texture);
        gl.uniform1i(uniformLocation(gl, p, "src"), 0);
        this.ctx.drawQuad();

        const data = new Uint8Array(width * height * 4);
        gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, data);
        gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        return { width, height, data };
    }

    /** @internal Read a node texture as float RGBA. */
    readFloat32(src: PooledTexture): ChannelPixels<Float32Array> {
        const gl = this.gl;
        const { width, height } = src;
        const data = new Float32Array(width * height * 4);
        if (this.ctx.format === "rgba8") {
            const bytes = this.readUint8(src).data;
            for (let i = 0; i < bytes.length; i++) data[i] = bytes[i]! / 255;
            return { width, height, data };
        }
        this.ctx.bindTarget(src);
        gl.readPixels(0, 0, width, height, gl.RGBA, gl.FLOAT, data);
        gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        return { width, height, data };
    }

    private getCopyProgram(): Program {
        if (!this.copyProgram) {
            const p = this.programs.get({
                source: `#version 300 es
precision highp float;
uniform sampler2D src;
in vec2 v_texCoord;
out vec4 fragColor;
void main() { fragColor = texture(src, v_texCoord); }
`,
                prefixLines: 0,
            });
            if (p instanceof ShaderCompileError) throw p;
            this.copyProgram = p;
        }
        return this.copyProgram;
    }

    dispose(): void {
        const gl = this.gl;
        this.programs.dispose();
        this.cache.dispose();
        this.ctx.dispose();
        if (this.uint8Target) gl.deleteTexture(this.uint8Target.texture8);
        for (const p of this.imageCache.values()) p.then((t) => t && gl.deleteTexture(t));
        this.imageCache.clear();
        gl.getExtension("WEBGL_lose_context")?.loseContext();
    }
}

function usesCustomRenderer(node: TextureNode): boolean {
    return typeof node.createRenderer === "function" || (node.usesCpuProcessing && typeof node.cpuProcess === "function");
}

function clampResolution(requested: number, max: number): number {
    // Largest power of two <= min(requested, max), at least 32.
    const limit = Math.max(32, Math.min(requested, max));
    return 2 ** Math.floor(Math.log2(limit));
}

export function createRuntime(options?: RuntimeOptions): TextureRuntime {
    return new TextureRuntime(options);
}
