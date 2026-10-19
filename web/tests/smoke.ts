// Renders .texture files in the playground with headless Chrome and reports
// load errors, shader compile errors and unsupported nodes.
//
//   node tests/smoke.ts                  # the 10 example graphs
//   node tests/smoke.ts --nodes          # the per-node demo files too
//   node tests/smoke.ts --all-nodes      # compile + render every node type (v2 and v3)
//   node tests/smoke.ts --custom         # multi-pass and CPU nodes, fed by generators
//   node tests/smoke.ts --variables      # texture variables change the render
//   node tests/smoke.ts --headed         # watch it
//
// Screenshots land in tests/out/. Needs Chrome/Chromium installed; set
// CHROME_PATH if it isn't at a standard location.

import { spawn } from "node:child_process";
import { existsSync, mkdirSync, readdirSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { chromium } from "playwright-core";

const here = dirname(fileURLToPath(import.meta.url));
const webRoot = resolve(here, "..");
const assets = resolve(webRoot, "../public/assets");
const outDir = join(here, "out");
const port = 5198; // not 5199, so it can run alongside `yarn dev`

const args = process.argv.slice(2);
const includeNodes = args.includes("--nodes");
const allNodes = args.includes("--all-nodes");
const customNodes = args.includes("--custom");
const variablesMode = args.includes("--variables");
const headed = args.includes("--headed");
const res = args.find((a) => a.startsWith("--res="))?.slice(6) ?? "512";

const chromePath =
    process.env.CHROME_PATH ??
    ["/usr/bin/google-chrome", "/usr/bin/chromium", "/usr/bin/chromium-browser", "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"].find(
        existsSync,
    );
if (!chromePath) {
    console.error("Chrome not found; set CHROME_PATH");
    process.exit(2);
}

const list = (dir: string) =>
    readdirSync(join(assets, dir))
        .filter((f) => f.endsWith(".texture"))
        .map((f) => f.replace(/\.texture$/, ""))
        .sort();

const cases: { param: string; name: string }[] = allNodes || customNodes || variablesMode ? [] : [
    ...list("examples").map((name) => ({ param: `example=${encodeURIComponent(name)}`, name })),
    ...(includeNodes ? list("nodes").map((name) => ({ param: `node=${encodeURIComponent(name)}`, name: `node:${name}` })) : []),
];

mkdirSync(outDir, { recursive: true });

// Start the playground dev server.
// Run vite's binary directly (not through yarn) so server.kill() stops it.
const server = spawn(join(webRoot, "node_modules/.bin/vite"), ["--port", String(port), "--strictPort"], {
    cwd: join(webRoot, "apps/playground"),
    stdio: ["ignore", "pipe", "pipe"],
});
await new Promise<void>((resolveReady, reject) => {
    const timer = setTimeout(() => reject(new Error("vite didn't start")), 30000);
    server.stdout.on("data", (d: Buffer) => {
        if (d.toString().includes("Local:")) {
            clearTimeout(timer);
            resolveReady();
        }
    });
    server.on("exit", (code) => reject(new Error(`vite exited with ${code}`)));
});

const browser = await chromium.launch({
    executablePath: chromePath,
    headless: !headed,
    args: ["--enable-gpu", "--ignore-gpu-blocklist", "--use-angle=default"],
});
const page = await browser.newPage({ viewport: { width: 1200, height: 800 } });
const consoleErrors: string[] = [];
page.on("console", (msg) => {
    if (msg.type() === "error") consoleErrors.push(msg.text());
});
page.on("pageerror", (err) => consoleErrors.push(err.message));

interface Status {
    state: string;
    error?: string;
    warnings: { kind: string; message: string }[];
    channels: string[];
    timeMs?: number;
    nodesRendered?: number;
}

let failures = 0;
const unsupported = new Map<string, number>();

for (const c of cases) {
    consoleErrors.length = 0;
    await page.goto(`http://localhost:${port}/?${c.param}&res=${res}`);
    let status: Status;
    try {
        await page.waitForFunction(
            () => {
                const s = (window as unknown as { __playground?: Status }).__playground;
                return s && (s.state === "done" || s.state === "error");
            },
            undefined,
            { timeout: 60000 },
        );
        status = await page.evaluate(() => (window as unknown as { __playground: Status }).__playground);
    } catch {
        status = { state: "timeout", warnings: [], channels: [] };
    }
    await page.waitForTimeout(150);
    await page.screenshot({ path: join(outDir, `${c.name.replace(/[:/]/g, "_")}.png`) });

    const shaderErrors = status.warnings.filter((w) => w.kind === "shader-error");
    for (const w of status.warnings.filter((w) => w.kind === "unsupported-node" || w.kind === "unknown-node")) {
        const key = w.message.split(":")[0]!;
        unsupported.set(key, (unsupported.get(key) ?? 0) + 1);
    }
    const ok = status.state === "done" && shaderErrors.length === 0;
    if (!ok) failures++;

    const detail =
        status.state === "done"
            ? `${status.nodesRendered} nodes, ${Math.round(status.timeMs ?? 0)} ms, channels: ${status.channels.join(",") || "none"}`
            : status.error ?? status.state;
    console.log(`${ok ? "ok  " : "FAIL"} ${c.name.padEnd(28)} ${detail}`);
    for (const w of status.warnings) {
        if (w.kind !== "unsupported-node") console.log(`       ${w.kind}: ${w.message.split("\n").slice(0, 6).join("\n         ")}`);
    }
    for (const e of consoleErrors) console.log(`       console: ${e.slice(0, 300)}`);
}

// One-node graphs for every node type, rendered straight into the albedo channel.
if (allNodes) {
    await page.goto(`http://localhost:${port}/?res=256`);
    await page.waitForFunction(() => (window as unknown as { __playground?: unknown }).__playground);
    for (const version of ["v3", "v2"] as const) {
        const types = await page.evaluate(
            (v) => (window as unknown as { __playground: { supportedNodes(v: string): string[] } }).__playground.supportedNodes(v),
            version,
        );
        for (const typeName of types) {
            const doc = JSON.stringify({
                libraryVersion: version,
                nodes: [{ id: "n", typeName, properties: {} }],
                connections: [],
                editor: { textureChannels: { albedo: "n" } },
            });
            consoleErrors.length = 0;
            const status = (await page.evaluate(async (text) => {
                const pg = (window as unknown as { __playground: Status & { render(n: string, t: string): Promise<void> } }).__playground;
                await pg.render("node", text);
                return JSON.parse(JSON.stringify(pg)) as Status;
            }, doc)) as Status;
            const errors = status.warnings.filter((w) => w.kind === "shader-error");
            const skipped = status.warnings.filter((w) => w.kind === "unsupported-node");
            const ok = status.state === "done" && errors.length === 0;
            if (!ok) failures++;
            cases.push({ param: "", name: `${version}:${typeName}` });
            if (!ok || skipped.length) {
                console.log(`${ok ? "skip" : "FAIL"} ${version}:${typeName}`);
                for (const w of [...errors, ...skipped]) console.log(`       ${w.message.split("\n").slice(0, 8).join("\n         ")}`);
                if (status.error) console.log(`       ${status.error}`);
            }
            for (const e of consoleErrors) console.log(`       console: ${e.slice(0, 300)}`);
        }
    }
}

// Multi-pass and CPU nodes with real inputs: generator -> node [-> viewer node].
if (customNodes) {
    type Chain = { version: string; name: string; nodes: [string, Record<string, unknown>][]; links: [number, number, string][] };
    const chains: Chain[] = [
        { version: "v3", name: "blurhq", nodes: [["checkerboard", {}], ["blurhq", { radius: 8 }]], links: [[0, 1, "image"]] },
        { version: "v3", name: "autolevels", nodes: [["fractalnoise", {}], ["autolevels", {}]], links: [[0, 1, "image"]] },
        { version: "v3", name: "distancetransform", nodes: [["polygon", { radius: 0.5 }], ["distancetransform", {}]], links: [[0, 1, "mask"]] },
        {
            version: "v3",
            name: "spread",
            nodes: [["fractalnoise", {}], ["brickgenerator", {}], ["spread", { threshold: 0.5 }]],
            links: [[0, 2, "color"], [1, 2, "mask"]],
        },
        { version: "v3", name: "bevelv2", nodes: [["polygon", { radius: 0.5 }], ["bevelv2", { distance: 40 }]], links: [[0, 1, "image"]] },
        { version: "v3", name: "fastao", nodes: [["brickgenerator", {}], ["fastao", { radius: 0.05 }]], links: [[0, 1, "height"]] },
        {
            version: "v3",
            name: "floodfillv2",
            nodes: [["brickgenerator", {}], ["floodfillv2", {}], ["floodfillv2torandomcolor", {}]],
            links: [[0, 1, "image"], [1, 2, "floodfill"]],
        },
        { version: "v2", name: "bevel", nodes: [["polygon", { radius: 0.5 }], ["bevel", { distance: 20 }]], links: [[0, 1, "image"]] },
        {
            version: "v2",
            name: "floodfill",
            nodes: [["brickgenerator", {}], ["floodfill", {}], ["floodfilltorandomcolor", {}]],
            links: [[0, 1, "image"], [1, 2, "floodfill"]],
        },
    ];

    await page.goto(`http://localhost:${port}/?res=${res}`);
    await page.waitForFunction(() => (window as unknown as { __playground?: unknown }).__playground);
    for (const chain of chains) {
        const doc = JSON.stringify({
            libraryVersion: chain.version,
            nodes: chain.nodes.map(([typeName, properties], i) => ({ id: `n${i}`, typeName, properties })),
            connections: chain.links.map(([l, r, input]) => ({ leftNodeId: `n${l}`, rightNodeId: `n${r}`, rightNodeInput: input })),
            editor: { textureChannels: { albedo: `n${chain.nodes.length - 1}`, height: `n${chain.nodes.length - 1}` } },
        });
        consoleErrors.length = 0;
        const status = (await page.evaluate(async ([name, text]) => {
            const pg = (window as unknown as { __playground: Status & { render(n: string, t: string): Promise<void> } }).__playground;
            await pg.render(name, text);
            return JSON.parse(JSON.stringify(pg)) as Status;
        }, [`${chain.version}:${chain.name}`, doc] as const)) as Status;
        await page.waitForTimeout(150);
        await page.screenshot({ path: join(outDir, `custom_${chain.version}_${chain.name}.png`) });
        const problems = status.warnings.filter((w) => w.kind !== "missing-channel");
        const ok = status.state === "done" && problems.length === 0;
        if (!ok) failures++;
        cases.push({ param: "", name: chain.name });
        console.log(`${ok ? "ok  " : "FAIL"} ${`${chain.version}:${chain.name}`.padEnd(28)} ${Math.round(status.timeMs ?? 0)} ms`);
        for (const w of problems) console.log(`       ${w.kind}: ${w.message.split("\n").slice(0, 8).join("\n         ")}`);
        if (status.error) console.log(`       ${status.error}`);
        for (const e of consoleErrors) console.log(`       console: ${e.slice(0, 300)}`);
    }
}

// Variables: every variable in Copper, plus a fixture for bool and enum (with
// a restricted, relabelled option list). Each change must re-render and
// change the output; reset must restore it.
if (variablesMode) {
    type PG = {
        render(n: string, t: string): Promise<void>;
        setVariable(name: string, value: unknown): Promise<void>;
        variables(): { name: string; type: string; value: unknown; min?: number; max?: number; options?: { index: number }[] }[];
        channelHash(channel: string): number | null;
        state: string;
    };
    const pg = <T>(fn: (pg: PG, arg: any) => T | Promise<T>, arg?: unknown) =>
        page.evaluate(([src, a]) => new Function("pg", "a", `return (${src})(pg, a)`)((window as any).__playground, a), [fn.toString(), arg] as const) as Promise<T>;
    const hashes = () => pg((p) => ["albedo", "normal", "roughness", "height"].map((c) => p.channelHash(c)).join(","));
    const check = (name: string, ok: boolean, detail = "") => {
        if (!ok) failures++;
        cases.push({ param: "", name });
        console.log(`${ok ? "ok  " : "FAIL"} ${name.padEnd(36)} ${detail}`);
    };

    await page.goto(`http://localhost:${port}/?example=Copper&res=${res}`);
    await page.waitForFunction(() => (window as any).__playground?.state === "done", undefined, { timeout: 60000 });
    const vars = await pg((p) => p.variables());
    check("copper has variables", vars.length === 5, vars.map((v) => v.name).join(", "));
    const base = await hashes();
    for (const v of vars) {
        const value =
            v.type === "color" ? { r: 0.2, g: 0.6, b: 0.9, a: 1 } : v.type === "float" || v.type === "int" ? (v.max as number) : v.value;
        await pg((p, a) => p.setVariable(a.name, a.value), { name: v.name, value });
        const after = await hashes();
        check(`copper:${v.name}`, after !== base, `-> ${JSON.stringify(value)}`);
        await pg((p, a) => p.setVariable(a.name, a.value), { name: v.name, value: v.value });
        const restored = await hashes();
        check(`copper:${v.name} restore`, restored === base);
    }
    // Out-of-bounds values clamp.
    await pg((p) => p.setVariable("wear_steps", 999));
    const steps = await pg((p) => p.variables().find((v) => v.name === "wear_steps")!.value);
    check("copper:clamp", steps === 12, `999 -> ${steps}`);

    // Presets: Copper ships "Polished" and "Worn".
    const presets = await pg((p: any) => p.presets().map((x: any) => ({ id: x.id, name: x.name })));
    check("copper presets", presets.length === 2, presets.map((x: any) => x.name).join(", "));
    await pg((p: any) => p.selectPreset(""));
    check("preset:saved values restores", (await hashes()) === base);
    await pg((p: any, a) => p.selectPreset(a), presets.find((x: any) => x.name === "Worn")!.id);
    const worn = await hashes();
    const wornVars = await pg((p) => p.variables().map((v) => [v.name, v.value]));
    check("preset:worn changes render", worn !== base, JSON.stringify(wornVars).slice(0, 90));
    check("preset:not modified after apply", !(await pg((p: any) => p.presetModified())));
    await pg((p) => p.setVariable("roughness", 0.5));
    check("preset:modified after edit", await pg((p: any) => p.presetModified()));
    await pg((p: any) => p.selectPreset(""));
    check("preset:back to saved values", (await hashes()) === base);

    // Fixture covering the other types: enum (numeric legacy type, restricted
    // and relabelled options), bool, gradient, curve, image and string.
    const png =
        "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAQAAAAECAYAAACp8Z5+AAAAG0lEQVR4nGNgYPj//z8Dw384jcJh+P+fgaAKACxQH+FG9T9gAAAAAElFTkSuQmCC";
    const fixture = JSON.stringify({
        libraryVersion: "v3",
        nodes: [
            { id: "g", typeName: "gradient", properties: {} },
            { id: "gm", typeName: "gradientmap", properties: {} },
            { id: "c", typeName: "cell", properties: {} },
            { id: "cv", typeName: "curve", properties: {} },
            { id: "img", typeName: "image", properties: {} },
            { id: "cp", typeName: "copy", properties: {} },
        ],
        connections: [
            { leftNodeId: "g", rightNodeId: "gm", rightNodeInput: "inputImage" },
            { leftNodeId: "g", rightNodeId: "cv", rightNodeInput: "image" },
        ],
        editor: { textureChannels: { albedo: "gm", roughness: "c", height: "cv", normal: "img" } },
        variables: [
            {
                id: "dir", type: 4,
                property: { name: "direction", displayName: "Direction", index: 0, values: ["Left To Right", "Right To Left", "Top To Bottom", "Bottom To Top"] },
                options: [{ index: 0, label: "Horizontal" }, { index: 2, label: "Vertical" }],
                linkedProperties: [{ nodeId: "g", name: "mode" }],
            },
            { id: "inv", type: "bool", property: { name: "invert", displayName: "Invert", value: false }, linkedProperties: [{ nodeId: "c", name: "invert" }] },
            {
                id: "ramp", type: "gradient",
                property: { name: "ramp", displayName: "Ramp", points: [
                    { t: 0, color: { r: 0, g: 0, b: 0, a: 1 } }, { t: 1, color: { r: 1, g: 1, b: 1, a: 1 } }] },
                linkedProperties: [{ nodeId: "gm", name: "gradient" }],
            },
            { id: "shape", type: "curve", property: { name: "shape", displayName: "Shape" }, linkedProperties: [{ nodeId: "cv", name: "curve" }] },
            { id: "decal", type: "image", property: { name: "decal", displayName: "Decal", value: "" }, linkedProperties: [{ nodeId: "img", name: "image" }] },
            { id: "label", type: "string", property: { name: "label", displayName: "Label", value: "hello" }, linkedProperties: [{ nodeId: "cp", name: "name" }] },
        ],
    });
    await pg((p, a) => p.render("fixture", a), fixture);
    const fx = await pg((p) => p.variables());
    check(
        "fixture parsed",
        fx.map((v) => v.type).join() === "enum,bool,gradient,curve,image,string" && fx.every((v: any) => v.links.length === 1),
        fx.map((v) => v.type).join(),
    );
    let prev = await hashes();
    const step = async (label: string, name: string, value: unknown) => {
        await pg((p, a) => p.setVariable(a.name, a.value), { name, value });
        const now = await hashes();
        check(label, now !== prev);
        prev = now;
    };
    await step("fixture:enum changes render", "direction", 2);
    await pg((p) => p.setVariable("direction", 1)); // hidden option -> first enabled
    const dir = await pg((p) => p.variables()[0]!.value);
    check("fixture:enum hidden option clamps", dir === 0, `1 -> ${dir}`);
    prev = await hashes();
    await step("fixture:bool changes render", "invert", true);
    // Gradient/curve values are built in the page (the runtime's classes).
    await pg(async (p) => {
        const v = p.variables().find((x) => x.name === "ramp")! as any;
        const g = v.value.clone();
        g.points[0].color = { r: 1, g: 0, b: 0, a: 1 };
        await p.setVariable("ramp", g);
    });
    let now = await hashes();
    check("fixture:gradient changes render", now !== prev);
    prev = now;
    await pg(async (p) => {
        const v = p.variables().find((x) => x.name === "shape")! as any;
        const c = v.value.clone();
        c.points[0].y = 1;
        c.points[1].y = 0;
        await p.setVariable("shape", c);
    });
    now = await hashes();
    check("fixture:curve changes render", now !== prev);
    prev = now;
    await step("fixture:image changes render", "decal", png);
    await pg((p) => p.setVariable("label", "renamed"));
    const label = await pg((p) => p.variables().find((x) => x.name === "label")!.value);
    check("fixture:string value set", label === "renamed");
    await page.setViewportSize({ width: 1200, height: 1400 });
    await page.screenshot({ path: join(outDir, "variables_fixture.png") });
}

if (unsupported.size) {
    console.log("\nUnsupported / unknown nodes encountered:");
    for (const [k, n] of unsupported) console.log(`  ${k} (${n} file${n > 1 ? "s" : ""})`);
}

const renderer = await page.evaluate(() => {
    const gl = document.createElement("canvas").getContext("webgl2");
    const ext = gl?.getExtension("WEBGL_debug_renderer_info");
    return ext ? gl!.getParameter(ext.UNMASKED_RENDERER_WEBGL) : "unknown";
});
console.log(`\n${cases.length - failures}/${cases.length} passed (GPU: ${renderer}). Screenshots: ${outDir}`);

await browser.close();
server.kill();
process.exit(failures ? 1 : 0);
