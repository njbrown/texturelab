import {
    applyPreset,
    createRuntime,
    parseTexture,
    resetVariables,
    setVariable,
    supportedNodes,
    TextureParseError,
    type ChannelPixels,
    type Color,
    Curve,
    type EvaluationResult,
    Gradient,
    type TextureChannel,
    type TextureDocument,
    type TextureMaps,
    type TextureVariable,
    type VariableValue,
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
    variablesGroup: $("variables-group"),
    variables: $("variables"),
    preset: $<HTMLSelectElement>("preset"),
    presetModified: $("preset-modified"),
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
    /** Set a variable and wait for the re-render (smoke test). */
    setVariable: async (name: string, value: VariableValue) => {
        if (!currentDoc) throw new Error("no document");
        setVariable(currentDoc, name, value);
        updatePresetModified();
        await requestRender({ preview: false, showProgress: false });
    },
    variables: () => currentDoc?.variables ?? [],
    presets: () => currentDoc?.presets ?? [],
    /** Pick a preset in the dropdown ("" = saved values) and wait for the render (smoke test). */
    selectPreset: async (id: string) => {
        ui.preset.value = id;
        ui.preset.dispatchEvent(new Event("change"));
        await requestRender({ preview: false, showProgress: false });
    },
    presetModified: () => ui.presetModified.textContent === "•",
    /** FNV-1a hash of a channel's pixels, to tell whether a render changed (smoke test). */
    channelHash: (channel: TextureChannel) => {
        const px = result?.readUint8(channel);
        if (!px) return null;
        let h = 0x811c9dc5;
        for (let i = 0; i < px.data.length; i++) h = Math.imul(h ^ px.data[i]!, 0x01000193);
        return h >>> 0;
    },
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

let currentDoc: TextureDocument | null = null;
// Bumped when a new file loads; renders of an older document are discarded.
let generation = 0;

/** Load a file: parse it, build its variable controls and render it. */
async function render(name: string, text: string): Promise<void> {
    current = { name, text };
    generation++;
    abort?.abort();

    Object.assign(status, { state: "rendering", file: name, error: undefined, warnings: [], channels: [] });
    ui.empty.hidden = true;
    ui.infoGroup.hidden = false;
    ui.fileName.textContent = name;
    ui.warnings.replaceChildren();

    try {
        currentDoc = parseTexture(text);
    } catch (e) {
        currentDoc = null;
        const message = e instanceof TextureParseError ? e.message : `Couldn't load: ${(e as Error).message}`;
        renderStats([]);
        ui.channels.replaceChildren();
        renderVariables(null);
        renderWarnings([{ kind: "error", message }], true);
        Object.assign(status, { state: "error", error: message });
        return;
    }

    renderVariables(currentDoc);
    await requestRender({ preview: false, showProgress: true });
}

// --- Render queue ---------------------------------------------------------------
// One evaluation at a time. Requests made while one runs collapse into a
// single follow-up render of the latest state, so dragging a variable slider
// renders as fast as the GPU allows without piling up work.

interface RenderRequest {
    /** Half resolution, for slider drags. */
    preview: boolean;
    showProgress: boolean;
}
let pendingRequest: RenderRequest | null = null;
let pumping: Promise<void> | null = null;

function requestRender(request: RenderRequest): Promise<void> {
    status.state = "rendering";
    // A full-quality request wins over a preview one queued before it.
    pendingRequest = pendingRequest && !pendingRequest.preview ? pendingRequest : request;
    if (!pumping) {
        pumping = (async () => {
            while (pendingRequest) {
                const next = pendingRequest;
                pendingRequest = null;
                await evaluateCurrent(next);
            }
            pumping = null;
        })();
    }
    return pumping;
}

async function evaluateCurrent(request: RenderRequest): Promise<void> {
    const doc = currentDoc;
    if (!doc) return;
    const gen = generation;
    const controller = (abort = new AbortController());
    if (request.showProgress) showProgress(0, 1);

    try {
        const chosen = ui.resolution.value === "file" ? doc.width : Number(ui.resolution.value);
        const resolution = request.preview ? Math.max(128, chosen / 2) : chosen;

        const next = await runtime.evaluate(doc, {
            resolution,
            signal: controller.signal,
            onProgress: request.showProgress ? ({ done, total }) => showProgress(done, total) : undefined,
        });
        if (controller.signal.aborted || gen !== generation) {
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
        ui.warnings.replaceChildren();
        renderWarnings(next.warnings.map((w) => ({ kind: w.kind, message: w.message })));
        if (next.channels.length === 0) {
            renderWarnings([{ kind: "missing-channel", message: "This file has no texture channels assigned, so there's nothing to preview." }]);
        }

        Object.assign(status, {
            state: pendingRequest ? "rendering" : "done",
            warnings: next.warnings.map((w) => ({ kind: w.kind, message: w.message })),
            channels: next.channels,
            timeMs: next.timeMs,
            nodesRendered: next.nodesRendered,
        });
    } catch (e) {
        if (controller.signal.aborted || gen !== generation) return;
        const message = `Render failed: ${(e as Error).message}`;
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
    void requestRender({ preview: false, showProgress: true });
}

// --- Variables ------------------------------------------------------------------

function renderVariables(doc: TextureDocument | null): void {
    const vars = doc?.variables ?? [];
    ui.variablesGroup.hidden = vars.length === 0;
    ui.variables.replaceChildren(...vars.map((v) => variableControl(v)));

    // "Saved values" (what the file was saved with) plus the file's presets.
    ui.preset.replaceChildren(
        new Option("Saved values", ""),
        ...(doc?.presets ?? []).map((p) => new Option(p.name, p.id)),
    );
    ui.preset.value = doc?.activePreset ?? "";
    updatePresetModified();
}

/** Show • when the values differ from the selected preset (or the saved values). */
function updatePresetModified(): void {
    const doc = currentDoc;
    if (!doc) return;
    const preset = doc.presets.find((p) => p.id === doc.activePreset);
    const key = (v: unknown) => JSON.stringify(v);
    const modified = doc.variables.some((v) => {
        const expected = preset ? preset.values[v.id] : v.defaultValue;
        return expected !== undefined && key(expected) !== key(v.value);
    });
    ui.presetModified.textContent = modified ? "•" : "";
}

ui.preset.addEventListener("change", () => {
    if (!currentDoc) return;
    if (ui.preset.value) applyPreset(currentDoc, ui.preset.value);
    else resetVariables(currentDoc);
    renderVariables(currentDoc);
    rerender();
});

/** A control for one variable. Sliders render at half resolution while dragged. */
function variableControl(variable: TextureVariable): HTMLElement {
    const set = (value: VariableValue, preview: boolean) => {
        if (!currentDoc) return;
        setVariable(currentDoc, variable.name, value);
        updatePresetModified();
        void requestRender({ preview, showProgress: false });
    };

    const field = document.createElement("label");
    field.className = "field";
    field.dataset.variable = variable.name;
    const caption = document.createElement("span");
    caption.textContent = variable.displayName;
    field.append(caption);

    switch (variable.type) {
        case "float":
        case "int": {
            field.classList.add("slider");
            const output = document.createElement("output");
            caption.append(output);
            const input = document.createElement("input");
            input.type = "range";
            const min = variable.min ?? 0;
            const max = variable.max ?? 1;
            input.min = String(min);
            input.max = String(max);
            input.step =
                variable.type === "int"
                    ? String(Math.max(1, variable.step ?? 1))
                    : String(variable.step && variable.step > 0 ? Math.min(variable.step, (max - min) / 100) : (max - min) / 100);
            input.value = String(variable.value);
            const fmt = (v: number) => (variable.type === "int" ? String(v) : v.toFixed(3).replace(/0+$/, "").replace(/\.$/, ""));
            output.textContent = fmt(variable.value as number);
            input.addEventListener("input", () => {
                output.textContent = fmt(Number(input.value));
                set(Number(input.value), true);
            });
            input.addEventListener("change", () => set(Number(input.value), false));
            field.append(input);
            break;
        }
        case "bool": {
            field.classList.add("toggle");
            const input = document.createElement("input");
            input.type = "checkbox";
            input.checked = variable.value as boolean;
            input.addEventListener("change", () => set(input.checked, false));
            field.prepend(input);
            break;
        }
        case "enum": {
            const select = document.createElement("select");
            for (const option of variable.options ?? []) select.append(new Option(option.label, String(option.index)));
            select.value = String(variable.value);
            select.addEventListener("change", () => set(Number(select.value), false));
            field.append(select);
            break;
        }
        case "color": {
            const input = document.createElement("input");
            input.type = "color";
            const c = variable.value as Color;
            const hex = (x: number) => Math.round(Math.min(1, Math.max(0, x)) * 255).toString(16).padStart(2, "0");
            input.value = `#${hex(c.r)}${hex(c.g)}${hex(c.b)}`;
            const parse = (): Color => ({
                r: parseInt(input.value.slice(1, 3), 16) / 255,
                g: parseInt(input.value.slice(3, 5), 16) / 255,
                b: parseInt(input.value.slice(5, 7), 16) / 255,
                a: (variable.value as Color).a,
            });
            input.addEventListener("input", () => set(parse(), true));
            input.addEventListener("change", () => set(parse(), false));
            field.append(input);
            break;
        }
        case "gradient":
            field.append(gradientEditor(variable.value as Gradient, set));
            break;
        case "curve":
            field.append(curveEditor(variable.value as Curve, set));
            break;
        case "image":
            field.append(imageEditor(variable.value as string, set));
            break;
        case "string": {
            const input = document.createElement("input");
            input.type = "text";
            input.value = variable.value as string;
            input.addEventListener("change", () => set(input.value, false));
            field.append(input);
            break;
        }
    }
    return field;
}

type SetValue = (value: VariableValue, preview: boolean) => void;

const toHex = (c: Color) => {
    const h = (x: number) => Math.round(Math.min(1, Math.max(0, x)) * 255).toString(16).padStart(2, "0");
    return `#${h(c.r)}${h(c.g)}${h(c.b)}`;
};
const fromHex = (hex: string, a = 1): Color => ({
    r: parseInt(hex.slice(1, 3), 16) / 255,
    g: parseInt(hex.slice(3, 5), 16) / 255,
    b: parseInt(hex.slice(5, 7), 16) / 255,
    a,
});

/** Preview bar plus a color + position control per stop. */
function gradientEditor(initial: Gradient, set: SetValue): HTMLElement {
    const gradient = initial.clone();
    const box = document.createElement("div");
    box.className = "gradient-editor";
    const bar = document.createElement("div");
    bar.className = "gradient-bar";
    const paint = () => {
        const stops = [...gradient.points].sort((a, b) => a.position - b.position);
        bar.style.background = stops.length
            ? `linear-gradient(to right, ${stops.map((p) => `${toHex(p.color)} ${(p.position * 100).toFixed(1)}%`).join(", ")})`
            : "#000";
    };
    paint();
    box.append(bar);

    gradient.points.forEach((point, i) => {
        const row = document.createElement("div");
        row.className = "gradient-stop";
        const color = document.createElement("input");
        color.type = "color";
        color.value = toHex(point.color);
        const pos = document.createElement("input");
        pos.type = "range";
        pos.min = "0";
        pos.max = "1";
        pos.step = "0.005";
        pos.value = String(point.position);
        const update = (preview: boolean) => {
            gradient.points[i] = { position: Number(pos.value), color: fromHex(color.value, point.color.a) };
            paint();
            set(gradient.clone(), preview);
        };
        for (const input of [color, pos]) {
            input.addEventListener("input", () => update(true));
            input.addEventListener("change", () => update(false));
        }
        row.append(color, pos);
        box.append(row);
    });
    return box;
}

/** SVG preview plus sliders for each anchor (interior anchors move in x too). */
function curveEditor(initial: Curve, set: SetValue): HTMLElement {
    const curve = initial.clone();
    const box = document.createElement("div");
    box.className = "curve-editor";
    const svgNS = "http://www.w3.org/2000/svg";
    const svg = document.createElementNS(svgNS, "svg");
    svg.setAttribute("viewBox", "-0.02 -0.02 1.04 1.04");
    svg.setAttribute("class", "curve-preview");
    svg.setAttribute("preserveAspectRatio", "none");
    const path = document.createElementNS(svgNS, "path");
    svg.append(path);
    const paint = () => {
        // y up: flip.
        const pt = (x: number, y: number) => `${x.toFixed(4)} ${(1 - y).toFixed(4)}`;
        const p = curve.points;
        let d = `M ${pt(p[0]!.x, p[0]!.y)}`;
        for (let i = 0; i < p.length - 1; i++) {
            const a = p[i]!;
            const b = p[i + 1]!;
            d += ` C ${pt(a.x + a.rx, a.y + a.ry)}, ${pt(b.x + b.lx, b.y + b.ly)}, ${pt(b.x, b.y)}`;
        }
        path.setAttribute("d", d);
    };
    paint();
    box.append(svg);

    curve.points.forEach((point, i) => {
        const interior = i > 0 && i < curve.points.length - 1;
        const row = document.createElement("div");
        row.className = "curve-anchor";
        const label = document.createElement("span");
        label.textContent = `Point ${i + 1}`;
        row.append(label);
        const slider = (axis: "x" | "y") => {
            const input = document.createElement("input");
            input.type = "range";
            input.min = "0";
            input.max = "1";
            input.step = "0.005";
            input.value = String(point[axis]);
            input.title = axis;
            const update = (preview: boolean) => {
                let v = Number(input.value);
                if (axis === "x") {
                    // Keep anchors ordered.
                    v = Math.min(curve.points[i + 1]!.x - 0.01, Math.max(curve.points[i - 1]!.x + 0.01, v));
                }
                curve.points[i]![axis] = v;
                paint();
                set(curve.clone(), preview);
            };
            input.addEventListener("input", () => update(true));
            input.addEventListener("change", () => update(false));
            return input;
        };
        if (interior) row.append(slider("x"));
        row.append(slider("y"));
        box.append(row);
    });
    return box;
}

/** Thumbnail plus a file picker that replaces the image. */
function imageEditor(initial: string, set: SetValue): HTMLElement {
    const box = document.createElement("div");
    box.className = "image-editor";
    const img = document.createElement("img");
    img.alt = "";
    if (initial) img.src = initial;
    const file = document.createElement("input");
    file.type = "file";
    file.accept = "image/*";
    file.hidden = true;
    const button = document.createElement("button");
    button.type = "button";
    button.className = "small";
    button.textContent = initial ? "Replace…" : "Choose image…";
    button.addEventListener("click", () => file.click());
    file.addEventListener("change", () => {
        const f = file.files?.[0];
        if (!f) return;
        const reader = new FileReader();
        reader.onload = () => {
            const url = String(reader.result);
            img.src = url;
            button.textContent = "Replace…";
            set(url, false);
        };
        reader.readAsDataURL(f);
    });
    box.append(img, button, file);
    return box;
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
