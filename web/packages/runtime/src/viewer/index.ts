// Three.js PBR preview of an evaluated texture. Independent of the GL
// runtime: it takes plain pixel data (EvaluationResult.toMaps()), so the
// runtime can later move to a worker without touching the viewer.

import {
    ACESFilmicToneMapping,
    BoxGeometry,
    Color,
    CylinderGeometry,
    DataTexture,
    FloatType,
    LinearFilter,
    LinearMipmapLinearFilter,
    Mesh,
    MeshStandardMaterial,
    NoColorSpace,
    PerspectiveCamera,
    PlaneGeometry,
    PMREMGenerator,
    RepeatWrapping,
    RGBAFormat,
    Scene,
    SphereGeometry,
    SRGBColorSpace,
    UnsignedByteType,
    Vector2,
    WebGLRenderer,
    type BufferGeometry,
    type ColorRepresentation,
    type Texture,
} from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls.js";
import type { ChannelPixels, TextureMaps } from "../core/index.js";
import {
    createGradientEnvironment,
    createStudioEnvironment,
    loadImageEnvironment,
    SKY_PRESETS,
    type Environment,
} from "./environment.js";

export {
    SKY_PRESETS,
    type Environment,
    type GradientEnvironment,
    type ImageEnvironment,
    type SkyPreset,
    type StudioEnvironment,
    type SunOptions,
} from "./environment.js";

export type MeshKind = "sphere" | "cube" | "plane" | "cylinder";
export type NormalConvention = "opengl" | "directx";
/** What's drawn behind the mesh: the environment, a blurred environment, or a flat color. */
export type Background = "environment" | "blurred" | ColorRepresentation;

export interface ViewerOptions {
    mesh?: MeshKind;
    /** Default: the gradient sky. */
    environment?: Environment;
    background?: Background;
    /**
     * Height map displacement along the normal, in mesh units (meshes are
     * about 2 units across). Default 0.05, which matches the desktop viewer's
     * 0.1 on its radius-2 sphere.
     */
    displacementScale?: number;
    normalConvention?: NormalConvention;
    /** UV repeat count. Default 1. */
    tiling?: number;
    exposure?: number;
}

export type ViewerEvent = "environmentError";

export class TextureViewer {
    readonly renderer: WebGLRenderer;
    readonly scene = new Scene();
    readonly camera: PerspectiveCamera;
    readonly controls: OrbitControls;

    private readonly container: HTMLElement;
    private readonly pmrem: PMREMGenerator;
    private readonly material = new MeshStandardMaterial({ color: 0xffffff, roughness: 0.5, metalness: 0 });
    private readonly mesh: Mesh;
    private readonly resizeObserver: ResizeObserver;
    private envTexture: Texture | null = null;
    private envRequest = 0;
    private mapTextures: Texture[] = [];
    private frameRequested = false;
    private displacementScale: number;
    private normalConvention: NormalConvention;
    private tiling: number;
    private background: Background;
    private listeners = new Map<ViewerEvent, Set<(detail: unknown) => void>>();
    private disposed = false;

    constructor(container: HTMLElement, options: ViewerOptions = {}) {
        this.container = container;
        this.displacementScale = options.displacementScale ?? 0.05;
        this.normalConvention = options.normalConvention ?? "opengl";
        this.tiling = options.tiling ?? 1;
        this.background = options.background ?? "blurred";

        this.renderer = new WebGLRenderer({ antialias: true });
        this.renderer.setPixelRatio(Math.min(globalThis.devicePixelRatio ?? 1, 2));
        this.renderer.toneMapping = ACESFilmicToneMapping;
        this.renderer.toneMappingExposure = options.exposure ?? 1;
        this.renderer.outputColorSpace = SRGBColorSpace;
        const canvas = this.renderer.domElement;
        canvas.style.display = "block";
        canvas.style.width = "100%";
        canvas.style.height = "100%";
        canvas.style.touchAction = "none";
        container.appendChild(canvas);

        this.pmrem = new PMREMGenerator(this.renderer);

        this.camera = new PerspectiveCamera(35, 1, 0.05, 100);
        this.camera.position.set(0, 0.4, 4.2);

        this.controls = new OrbitControls(this.camera, canvas);
        this.controls.enableDamping = true;
        this.controls.minDistance = 1.5;
        this.controls.maxDistance = 12;
        this.controls.addEventListener("change", () => this.requestRender());

        this.mesh = new Mesh(createGeometry(options.mesh ?? "sphere"), this.material);
        this.scene.add(this.mesh);

        this.resizeObserver = new ResizeObserver(() => this.resize());
        this.resizeObserver.observe(container);
        this.resize();

        void this.setEnvironment(options.environment ?? SKY_PRESETS.default);
    }

    /** Apply evaluated maps (from EvaluationResult.toMaps()). Missing channels fall back to constants. */
    setMaps(maps: TextureMaps): void {
        this.clearMaps(false);
        const m = this.material;

        m.map = maps.albedo ? this.texture(maps.albedo, true) : null;
        m.normalMap = maps.normal ? this.texture(maps.normal, false) : null;
        // Grayscale outputs carry the value in every channel; three reads
        // roughness from G and metalness from B.
        m.roughnessMap = maps.roughness ? this.texture(maps.roughness, false) : null;
        m.roughness = maps.roughness ? 1 : 0.5;
        m.metalnessMap = maps.metalness ? this.texture(maps.metalness, false) : null;
        m.metalness = maps.metalness ? 1 : 0;
        m.aoMap = maps.ao ? this.texture(maps.ao, false) : null;
        m.alphaMap = maps.alpha ? this.texture(maps.alpha, false) : null;
        m.transparent = !!maps.alpha;
        m.displacementMap = maps.height ? this.floatTexture(maps.height) : null;

        this.applyMaterialSettings();
        m.needsUpdate = true;
        this.requestRender();
    }

    clearMaps(render = true): void {
        for (const t of this.mapTextures) t.dispose();
        this.mapTextures = [];
        const m = this.material;
        m.map = m.normalMap = m.roughnessMap = m.metalnessMap = m.aoMap = m.alphaMap = m.displacementMap = null;
        m.roughness = 0.5;
        m.metalness = 0;
        m.transparent = false;
        m.needsUpdate = true;
        if (render) this.requestRender();
    }

    setMesh(kind: MeshKind): void {
        this.mesh.geometry.dispose();
        this.mesh.geometry = createGeometry(kind);
        this.requestRender();
    }

    async setEnvironment(env: Environment): Promise<void> {
        const request = ++this.envRequest;
        let texture: Texture;
        try {
            if (env.type === "gradient") texture = createGradientEnvironment(this.pmrem, env);
            else if (env.type === "studio") texture = createStudioEnvironment(this.pmrem);
            else texture = await loadImageEnvironment(this.pmrem, env);
        } catch (error) {
            // Fall back to the gradient sky so the mesh stays lit.
            this.emit("environmentError", error);
            texture = createGradientEnvironment(this.pmrem, SKY_PRESETS.default);
        }
        if (this.disposed || request !== this.envRequest) {
            texture.dispose();
            return;
        }
        this.envTexture?.dispose();
        this.envTexture = texture;
        this.scene.environment = texture;
        this.applyBackground();
        this.requestRender();
    }

    setBackground(background: Background): void {
        this.background = background;
        this.applyBackground();
        this.requestRender();
    }

    setDisplacementScale(scale: number): void {
        this.displacementScale = scale;
        this.applyMaterialSettings();
        this.requestRender();
    }

    setNormalConvention(convention: NormalConvention): void {
        this.normalConvention = convention;
        this.applyMaterialSettings();
        this.requestRender();
    }

    setTiling(tiling: number): void {
        this.tiling = tiling;
        for (const t of this.mapTextures) t.repeat.set(tiling, tiling);
        this.requestRender();
    }

    setExposure(exposure: number): void {
        this.renderer.toneMappingExposure = exposure;
        this.requestRender();
    }

    on(event: ViewerEvent, listener: (detail: unknown) => void): () => void {
        let set = this.listeners.get(event);
        if (!set) this.listeners.set(event, (set = new Set()));
        set.add(listener);
        return () => set.delete(listener);
    }

    resize(): void {
        const w = Math.max(1, this.container.clientWidth);
        const h = Math.max(1, this.container.clientHeight);
        this.renderer.setSize(w, h, false);
        this.camera.aspect = w / h;
        this.camera.updateProjectionMatrix();
        this.requestRender();
    }

    /** Render on demand; keeps going while orbit damping settles. */
    requestRender(): void {
        if (this.frameRequested || this.disposed) return;
        this.frameRequested = true;
        requestAnimationFrame(() => {
            this.frameRequested = false;
            if (this.disposed) return;
            const moving = this.controls.update();
            this.renderer.render(this.scene, this.camera);
            if (moving) this.requestRender();
        });
    }

    dispose(): void {
        this.disposed = true;
        this.resizeObserver.disconnect();
        this.controls.dispose();
        this.clearMaps(false);
        this.mesh.geometry.dispose();
        this.material.dispose();
        this.envTexture?.dispose();
        this.pmrem.dispose();
        this.renderer.dispose();
        this.renderer.domElement.remove();
        this.listeners.clear();
    }

    private emit(event: ViewerEvent, detail: unknown): void {
        for (const l of this.listeners.get(event) ?? []) l(detail);
    }

    private applyMaterialSettings(): void {
        const m = this.material;
        m.normalScale = new Vector2(1, this.normalConvention === "directx" ? -1 : 1);
        // Like the desktop viewer: displace outward by height * scale, no bias.
        m.displacementScale = this.displacementScale;
        m.displacementBias = 0;
    }

    private applyBackground(): void {
        const bg = this.background;
        if (bg === "environment" || bg === "blurred") {
            this.scene.background = this.envTexture;
            this.scene.backgroundBlurriness = bg === "blurred" ? 0.5 : 0;
            this.scene.backgroundIntensity = bg === "blurred" ? 0.6 : 1;
        } else {
            this.scene.background = new Color(bg);
        }
    }

    private texture(pixels: ChannelPixels<Uint8Array>, srgb: boolean): Texture {
        const t = new DataTexture(pixels.data, pixels.width, pixels.height, RGBAFormat, UnsignedByteType);
        t.colorSpace = srgb ? SRGBColorSpace : NoColorSpace;
        t.wrapS = t.wrapT = RepeatWrapping;
        t.magFilter = LinearFilter;
        t.minFilter = LinearMipmapLinearFilter;
        t.generateMipmaps = true;
        t.anisotropy = this.renderer.capabilities.getMaxAnisotropy();
        t.repeat.set(this.tiling, this.tiling);
        t.needsUpdate = true;
        this.mapTextures.push(t);
        return t;
    }

    private floatTexture(pixels: ChannelPixels<Float32Array>): Texture {
        const t = new DataTexture(pixels.data, pixels.width, pixels.height, RGBAFormat, FloatType);
        t.colorSpace = NoColorSpace;
        t.wrapS = t.wrapT = RepeatWrapping;
        t.magFilter = LinearFilter;
        t.minFilter = LinearFilter;
        t.generateMipmaps = false;
        t.repeat.set(this.tiling, this.tiling);
        t.needsUpdate = true;
        this.mapTextures.push(t);
        return t;
    }
}

function createGeometry(kind: MeshKind): BufferGeometry {
    // Dense enough for displacement to read as surface detail.
    switch (kind) {
        case "cube":
            return new BoxGeometry(1.6, 1.6, 1.6, 128, 128, 128);
        case "plane":
            return new PlaneGeometry(2.4, 2.4, 384, 384);
        case "cylinder":
            return new CylinderGeometry(0.9, 0.9, 2, 256, 128, true);
        case "sphere":
        default: {
            // The desktop sphere repeats UVs twice around the equator so
            // texels stay roughly square.
            const sphere = new SphereGeometry(1, 384, 192);
            const uv = sphere.getAttribute("uv");
            for (let i = 0; i < uv.count; i++) uv.setX(i, uv.getX(i) * 2);
            return sphere;
        }
    }
}

export function createViewer(container: HTMLElement, options?: ViewerOptions): TextureViewer {
    return new TextureViewer(container, options);
}
