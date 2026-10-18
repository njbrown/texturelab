import {
    createRuntime,
    parseTexture,
    supportedNodes,
    TextureParseError,
    type ChannelPixels,
    type EvaluationResult,
    type TextureChannel,
    type TextureMaps,
} from "@njbrown/texturelab-runtime";
import {
    createViewer,
    SKY_PRESETS,
    type Environment,
    type MeshKind,
    type NormalConvention,
    type SkyPreset,
} from "@njbrown/texturelab-runtime/viewer";

declare const __EXAMPLES__: string[];
declare const __NODE_DEMOS__: string[];
declare const __HDRIS__: string[];

const $ = <T extends HTMLElement>(id: string) => document.getElementById(id) as T;

const ui = {
    stage: $("stage"),
    empty: $("empty"),
    dropOverlay: $("drop-overlay"),
    progress: $("progress"),
    progressFill: $("progress-fill"),
    progressText: $("progress-text"),
    open: $<HTMLButtonElement>("open"),
    openEmpty: $<HTMLButtonElement>("open-empty"),
    file: $<HTMLInputElement>("file"),
    example: $<HTMLSelectElement>("example"),
    nodeDemo: $<HTMLSelectElement>("node-demo"),
    infoGroup: $("info-group"),
    fileName: $("file-name"),
    stats: $("stats"),
    channels: $("channels"),
    warnings: $("warnings"),
    resolution: $<HTMLSelectElement>("resolution"),
    mesh: $<HTMLSelectElement>("mesh"),
    environment: $<HTMLSelectElement>("environment"),
    background: $<HTMLSelectElement>("background"),
    normals: $<HTMLSelectElement>("normals"),
    displacement: $<HTMLInputElement>("displacement"),
    displacementValue: $<HTMLOutputElement>("displacement-value"),
    tiling: $<HTMLInputElement>("tiling"),
    tilingValue: $<HTMLOutputElement>("tiling-value"),
    exposure: $<HTMLInputElement>("exposure"),
    exposureValue: $<HTMLOutputElement>("exposure-value"),
};

// State exposed for the smoke test (tests/smoke.ts).
interface PlaygroundStatus {
    state: "idle" | "rendering" | "done" | "error";
    file?: string;
    error?: string;
    warnings: { kind: string; message: string }[];
    channels: TextureChannel[];
    timeMs?: number;
    nodesRendered?: number;
}
const status: PlaygroundStatus = { state: "idle", warnings: [], channels: [] };
(window as unknown as { __playground: unknown }).__playground = Object.assign(status, {
    supportedNodes,
    render: (name: string, text: string) => render(name, text),
});

// --- Runtime + viewer ---------------------------------------------------------

const runtime = createRuntime({ maxResolution: 2048 });
const viewer = createViewer($("viewer"), { mesh: "sphere" });
let current: { name: string; text: string } | null = null;
let result: EvaluationResult | null = null;
let abort: AbortController | null = null;

viewer.on("environmentError", (err) => {
    console.error("environment failed to load", err);
});

async function render(name: string, text: string): Promise<void> {
    current = { name, text };
    abort?.abort();
    const controller = (abort = new AbortController());

    Object.assign(status, { state: "rendering", file: name, error: undefined, warnings: [], channels: [] });
    ui.empty.hidden = true;
    ui.infoGroup.hidden = false;
    ui.fileName.textContent = name;
    ui.warnings.replaceChildren();
    showProgress(0, 1);

    try {
        const doc = parseTexture(text);
        const resolution = ui.resolution.value === "file" ? undefined : Number(ui.resolution.value);

        const next = await runtime.evaluate(doc, {
            resolution,
            signal: controller.signal,
            onProgress: ({ done, total }) => showProgress(done, total),
        });
        if (controller.signal.aborted) {
            next.dispose();
            return;
        }

        const maps = next.toMaps();
        result?.dispose();
        result = next;
        viewer.setMaps(maps);

        renderStats([
            ["Library", doc.libraryVersion],
            ["Nodes", `${next.nodesRendered} rendered / ${doc.nodes.size} total`],
            ["Resolution", `${next.width}×${next.height}` + (next.width !== doc.width ? ` (file: ${doc.width})` : "")],
            ["Time", `${Math.round(next.timeMs)} ms`],
            ["Precision", runtime.renderTargetFormat],
        ]);
        renderChannels(maps);
        renderWarnings(next.warnings.map((w) => ({ kind: w.kind, message: w.message })));
        if (next.channels.length === 0) {
            renderWarnings([{ kind: "missing-channel", message: "This file has no texture channels assigned, so there's nothing to preview." }]);
        }

        Object.assign(status, {
            state: "done",
            warnings: next.warnings.map((w) => ({ kind: w.kind, message: w.message })),
            channels: next.channels,
            timeMs: next.timeMs,
            nodesRendered: next.nodesRendered,
        });
    } catch (e) {
        if (controller.signal.aborted) return;
        const message = e instanceof TextureParseError ? e.message : `Render failed: ${(e as Error).message}`;
        console.error(e);
        renderStats([]);
        ui.channels.replaceChildren();
        renderWarnings([{ kind: "error", message }], true);
        Object.assign(status, { state: "error", error: message });
    } finally {
        if (abort === controller) ui.progress.hidden = true;
    }
}

function rerender(): void {
    if (current) void render(current.name, current.text);
}

// --- UI rendering ---------------------------------------------------------------

function showProgress(done: number, total: number): void {
    ui.progress.hidden = false;
    ui.progressFill.style.width = `${(100 * done) / Math.max(total, 1)}%`;
    ui.progressText.textContent = total > 1 ? `Rendering ${done}/${total} nodes` : "Compiling shaders…";
}

function renderStats(rows: [string, string][]): void {
    ui.stats.replaceChildren(
        ...rows.flatMap(([k, v]) => {
            const dt = document.createElement("dt");
            dt.textContent = k;
            const dd = document.createElement("dd");
            dd.textContent = v;
            return [dt, dd];
        }),
    );
}

function renderWarnings(items: { kind: string; message: string }[], isError = false): void {
    for (const item of items) {
        const li = document.createElement("li");
        li.textContent = item.message;
        if (isError || item.kind === "shader-error") li.classList.add("error");
        ui.warnings.append(li);
    }
}

const CHANNEL_LABELS: Record<TextureChannel, string> = {
    albedo: "Albedo",
    normal: "Normal",
    roughness: "Roughness",
    metalness: "Metal",
    height: "Height",
    ao: "AO",
    alpha: "Alpha",
};

function renderChannels(maps: TextureMaps): void {
    const tiles: HTMLElement[] = [];
    for (const channel of Object.keys(CHANNEL_LABELS) as TextureChannel[]) {
        const pixels = maps[channel];
        if (!pixels) continue;
        const tile = document.createElement("div");
        tile.className = "channel";
        tile.title = channel;
        tile.append(thumbnail(pixels), CHANNEL_LABELS[channel]);
        tiles.push(tile);
    }
    ui.channels.replaceChildren(...tiles);
}

/** Downsampled, top-row-first preview of a channel. */
function thumbnail(pixels: ChannelPixels<Uint8Array | Float32Array>): HTMLCanvasElement {
    const size = 128;
    const canvas = document.createElement("canvas");
    canvas.width = canvas.height = size;
    const ctx = canvas.getContext("2d")!;
    const img = ctx.createImageData(size, size);
    const scale = pixels.data instanceof Float32Array ? 255 : 1;
    for (let y = 0; y < size; y++) {
        // GL rows are bottom-to-top.
        const sy = pixels.height - 1 - Math.floor((y * pixels.height) / size);
        for (let x = 0; x < size; x++) {
            const sx = Math.floor((x * pixels.width) / size);
            const s = (sy * pixels.width + sx) * 4;
            const d = (y * size + x) * 4;
            img.data[d] = pixels.data[s]! * scale;
            img.data[d + 1] = pixels.data[s + 1]! * scale;
            img.data[d + 2] = pixels.data[s + 2]! * scale;
            img.data[d + 3] = 255;
        }
    }
    ctx.putImageData(img, 0, 0);
    return canvas;
}

// --- File input -----------------------------------------------------------------

async function openFile(file: File): Promise<void> {
    if (!file.name.endsWith(".texture") && file.type !== "application/json") {
        ui.empty.hidden = true;
        ui.infoGroup.hidden = false;
        ui.fileName.textContent = file.name;
        ui.warnings.replaceChildren();
        renderWarnings([{ kind: "error", message: "That isn't a .texture file." }], true);
        return;
    }
    ui.example.value = "";
    ui.nodeDemo.value = "";
    await render(file.name, await file.text());
}

async function openUrl(path: string): Promise<void> {
    const res = await fetch(`/${path}`);
    if (!res.ok) throw new Error(`Couldn't load ${path}`);
    await render(path.split("/").pop()!, await res.text());
}

ui.open.addEventListener("click", () => ui.file.click());
ui.openEmpty.addEventListener("click", () => ui.file.click());
ui.file.addEventListener("change", () => {
    const f = ui.file.files?.[0];
    if (f) void openFile(f);
    ui.file.value = "";
});

let dragDepth = 0;
window.addEventListener("dragenter", (e) => {
    if (!e.dataTransfer?.types.includes("Files")) return;
    e.preventDefault();
    dragDepth++;
    ui.dropOverlay.hidden = false;
});
window.addEventListener("dragleave", () => {
    dragDepth = Math.max(0, dragDepth - 1);
    if (dragDepth === 0) ui.dropOverlay.hidden = true;
});
window.addEventListener("dragover", (e) => e.preventDefault());
window.addEventListener("drop", (e) => {
    e.preventDefault();
    dragDepth = 0;
    ui.dropOverlay.hidden = true;
    const f = e.dataTransfer?.files[0];
    if (f) void openFile(f);
});

function fillSelect(select: HTMLSelectElement, placeholder: string, paths: string[], label: (p: string) => string): void {
    select.replaceChildren(new Option(placeholder, ""), ...paths.map((p) => new Option(label(p), p)));
}

const baseName = (p: string) => p.split("/").pop()!.replace(/\.texture$/, "");
fillSelect(ui.example, "Choose an example…", __EXAMPLES__, baseName);
fillSelect(ui.nodeDemo, "Choose a node…", __NODE_DEMOS__, baseName);

ui.example.addEventListener("change", () => {
    if (!ui.example.value) return;
    ui.nodeDemo.value = "";
    void openUrl(ui.example.value);
});
ui.nodeDemo.addEventListener("change", () => {
    if (!ui.nodeDemo.value) return;
    ui.example.value = "";
    void openUrl(ui.nodeDemo.value);
});

// --- Preview controls -----------------------------------------------------------

const presetLabels: Record<SkyPreset, string> = {
    default: "Sky (default)",
    overcast: "Sky: overcast",
    sunset: "Sky: sunset",
    night: "Sky: night",
};
ui.environment.replaceChildren(
    ...Object.entries(presetLabels).map(([k, v]) => new Option(v, `preset:${k}`)),
    new Option("Studio (generated)", "studio"),
    ...__HDRIS__.map((p) => new Option(`HDRI: ${baseName(p).replace(/_1k\.hdr$/, "").replace(/\.hdr$/, "")}`, `hdr:${p}`)),
);

ui.environment.addEventListener("change", () => {
    const v = ui.environment.value;
    let env: Environment;
    if (v.startsWith("preset:")) env = SKY_PRESETS[v.slice(7) as SkyPreset];
    else if (v === "studio") env = { type: "studio" };
    else env = { type: "hdr", url: `/${v.slice(4)}` };
    void viewer.setEnvironment(env);
});
ui.mesh.addEventListener("change", () => viewer.setMesh(ui.mesh.value as MeshKind));
ui.background.addEventListener("change", () => viewer.setBackground(ui.background.value));
ui.normals.addEventListener("change", () => viewer.setNormalConvention(ui.normals.value as NormalConvention));
ui.resolution.addEventListener("change", rerender);

function bindSlider(input: HTMLInputElement, output: HTMLOutputElement, apply: (v: number) => void, fmt = (v: number) => String(v)) {
    const update = () => {
        const v = Number(input.value);
        output.textContent = fmt(v);
        apply(v);
    };
    input.addEventListener("input", update);
    update();
}
bindSlider(ui.displacement, ui.displacementValue, (v) => viewer.setDisplacementScale(v), (v) => v.toFixed(3));
bindSlider(ui.tiling, ui.tilingValue, (v) => viewer.setTiling(v), (v) => `${v}×`);
bindSlider(ui.exposure, ui.exposureValue, (v) => viewer.setExposure(v), (v) => v.toFixed(2));

// ?example=Sand or ?node=perlinnoise opens a file directly (used by the smoke test).
const params = new URLSearchParams(location.search);
const exampleParam = params.get("example");
const nodeParam = params.get("node");
if (params.get("res")) ui.resolution.value = params.get("res")!;
if (exampleParam) {
    const path = __EXAMPLES__.find((p) => baseName(p) === exampleParam);
    if (path) {
        ui.example.value = path;
        void openUrl(path);
    }
} else if (nodeParam) {
    const path = __NODE_DEMOS__.find((p) => baseName(p) === nodeParam);
    if (path) {
        ui.nodeDemo.value = path;
        void openUrl(path);
    }
}
