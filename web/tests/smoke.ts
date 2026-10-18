// Renders .texture files in the playground with headless Chrome and reports
// load errors, shader compile errors and unsupported nodes.
//
//   node tests/smoke.ts                  # the 10 example graphs
//   node tests/smoke.ts --nodes          # the per-node demo files too
//   node tests/smoke.ts --all-nodes      # compile + render every node type (v2 and v3)
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
const port = 5199;

const args = process.argv.slice(2);
const includeNodes = args.includes("--nodes");
const allNodes = args.includes("--all-nodes");
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

const cases: { param: string; name: string }[] = allNodes ? [] : [
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
