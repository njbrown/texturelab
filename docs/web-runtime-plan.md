# TextureLab Web Runtime — Plan

**Goal (phase 1):** a web page where you drag a `.texture` file in, the graph is evaluated on the GPU in the browser, and the result is rendered as PBR on a sphere.

**End game:** a public gallery, along the lines of GameTextures or the Substance 3D community assets. Anyone can sign up, upload `.texture` files and see live 3D previews. They can also browse, download source graphs and exported maps, and eventually tweak parameters in the browser.

Every phase-1 decision below is made so it carries straight into the gallery.

---

## 1. What we're porting (current state)

The facts below come from the codebase and decide the architecture.

| Area | Desktop today | Implication for web |
|---|---|---|
| File format | Plain UTF-8 JSON (`project.cpp` `loadTextureFromJson` / `saveTexture`). `nodes[]`, `connections[]`, `libraryVersion` (v1/v2/v3), `textureWidth/Height`, `randomSeed`, `editor.textureChannels` maps channel → node id. Embedded images are `data:image/png;base64` strings inside ImageProps. Legacy files store some numbers as strings (`"strength":"0.2"`). | Trivial to parse in TS. A file can't carry shader code (string props only feed `copy`/`output`), so **uploads can't inject GLSL**, which matters a lot for the gallery. |
| GPU API | Raw OpenGL 3.2 Core, `#version 150 core`. One fullscreen pass per node. | GLSL 150 → GLSL ES 3.00 (WebGL2) is a mostly mechanical rewrite: `#version`, precision qualifiers, a few builtins. |
| Node shaders | About 108 v3 nodes in `src/texturelab/libraries/{v1,v2,v3}`. Each `init()` calls `addInput` / `add*Prop` / `setShaderSource(...)` with a `vec4 process(vec2 uv)` body. They share a prelude (`noderenderer.cpp`: preamble, `randomLib`, `gradientLib`, `curveLib`). Inputs become `sampler2D x` + `bool x_connected`. Props become `prop_<name>` uniforms. | **One TS class per node, like the Electron version** (§3.1). The C++ nodes are near line-for-line transliterations of the old TS ones, so porting is mechanical. A cheap CI check catches TS/C++ shader drift. |
| Non-trivial nodes | Custom multi-pass `NodeTextureRenderer`s in v3: `autolevels` (reduction), `blurhq` (separable), `fastao` (mipmaps), `distancetransform` / `bevelv2` / `spread` (JFA), `floodfillv2` (**CPU BFS**). v2 `bevel` / `floodfill` use CPU readback. | About 7–9 nodes need a hand-written TS renderer. Everything else is generic. |
| Scheduling | Dirty-flag scheduler (`texturerenderer.cpp` `getNextUpdatableNode`). Every node keeps a persistent **RGBA32F** texture at project resolution. Per-node seed = `project.randomSeed + node.randomSeed`. | Same model in TS. The web doesn't need every node's texture after evaluation, though, so release intermediates (§3.3). Otherwise a 2K, 40-node graph needs gigabytes of VRAM. |
| Library versions | `libraryVersion` picks the library. v1 and v2 both use `createLibraryV2()`. `LibraryVersionMigrator` renames some nodes from v2 to v3, and blend/cell nodes change how they look. | The web keeps **separate `v2/` and `v3/` libraries** (as desktop does) and files render with the library they declare. Gallery uploads are never silently migrated. |
| Viewer | `src/viewer3d`: port of the Khronos glTF-Sample-Renderer PBR, HDR IBL, sphere/cube/plane/cylinder meshes, height as displacement. HDRIs are in `public/assets/env/`. | Phase 1 uses three.js `MeshStandardMaterial` + PMREM image-based lighting. **No HDRIs ship in the package.** The default lighting is a generated gradient sky, and HDRIs are optional URLs served by the website or a CDN (§3.5). |
| Prior art | The original TextureLab was Vue/Electron with a WebGL runtime. It was deleted in `f62fa83` ("cleanup electron and web files", 2022), and `git show f62fa83^:src/lib/designer/gl.ts` and its neighbours still have it. | **Recover it as the starting point.** The old tree has `library/v1` (30 nodes) and `library/v2` (48 nodes) as TS classes (`GpuDesignerNode`, `addInput`, `buildShader`), plus `designer/gl.ts` and gradient/property code. Restore the v1/v2 node files and diff each one against the current C++ to pick up later fixes. Only the 43 v3 nodes need new ports. Modernise the plumbing (WebGL1 → WebGL2, float targets) rather than reviving it unchanged. |
| Test data | 10 example graphs in `public/assets/examples/*.texture`, plus about 107 per-node demo files in `public/assets/nodes/`. | A ready-made parity corpus. |

---

## 2. Architecture overview

```
                 CI check                                    browser
┌───────────────────────────────────────┐     ┌───────────────────────────────────────────┐
│ C++ node libraries (v2, v3)           │     │ @njbrown/texturelab-runtime  (framework-agnostic)  │
│        ⇅  shader drift check          │     │   parse → validate                         │
│ TS node libraries (v2, v3)            │──▶  │   schedule → WebGL2 passes → outputs       │
│   one class per node, Electron-style  │     │   TS node classes (incl. multi-pass ones)  │
└───────────────────────────────────────┘     ├───────────────────────────────────────────┤
                                              │ @texturelab/viewer   (three.js PBR)        │
                                              ├───────────────────────────────────────────┤
                                              │ apps/playground  (phase 1: drag & drop)    │
                                              └───────────────────────────────────────────┘
                                                               │ gh-publish-runtime.sh → GitHub Packages
                                                               ▼
                                              ┌───────────────────────────────────────────┐
                                              │ website repo (npm dep from GH Packages)    │
                                              │   gallery app + render worker (headless    │
                                              │   Chromium running the same runtime)       │
                                              └───────────────────────────────────────────┘
```

**Main choices:**

- **WebGL2, not WebGPU (for now).** The GLSL can be reused almost verbatim. WebGPU would need a WGSL translation step (naga/tint) and still has patchy support on Linux and mobile. The GPU backend sits behind a small interface so WebGPU can be added later.
- **Hand-written TS nodes, mirroring the C++ classes one-to-one** (same file names, same `init()` shape). This is simple to read and debug, and the web build needs no Qt or C++ toolchain, which keeps the release pipeline pure Node. Drift is caught by a source-level shader diff and the image parity harness.
- **Runtime as one standalone, framework-agnostic package** (`@njbrown/texturelab-runtime`). The **website repo installs it from GitHub Packages** (§2.1). The playground here, the gallery and render worker in the website repo, and a possible future embeddable `<texture-viewer>` widget all use it.
- **Source lives in this repo**, under `web/`, so a node change in C++ and its TS counterpart land in the same commit (the drift check enforces it). **The gallery lives in the website repo** and only depends on the built package.

### Repo layout

```
texturelab/ (this repo)
  web/
    package.json            (yarn workspace)
    packages/
      runtime/              THE distributable package: @njbrown/texturelab-runtime
        src/core/           parser, schema, scheduler, GL backend, custom nodes
        src/viewer/         three.js scene: procedural meshes + skies, PBR material binding
        src/library/        base classes, props, shader prelude, registry
          v2/               one .ts per node (restored from Electron history + synced)
          v3/               one .ts per node, ported from src/texturelab/libraries/v3
    apps/
      playground/           Vite + TS drag & drop page (phase 1), dogfoods the package
    scripts/
      port-node.ts          one-off C++ → TS scaffolder (raw-string GLSL + add*Prop calls)
      check-drift.ts        CI: compare normalised GLSL/props between C++ and TS nodes
    tests/
      parity/               desktop-vs-web golden image comparison

website/ (separate repo)
  package.json              "@njbrown/texturelab-runtime": "^0.1.0"   (+ .npmrc → npm.pkg.github.com)
  ...gallery app, render worker
```

### 2.1 Packaging and distribution (GitHub Packages)

**No submodules, no extra branches, no extra repos.** The runtime source stays in `web/` in this repo. A script builds it and publishes it to the **GitHub Packages npm registry**, attached to `njbrown/texturelab`. The website installs it like any other npm package.

**Package name:** GitHub Packages only accepts npm packages scoped to the repo owner, so the name is **`@njbrown/texturelab-runtime`**. `package.json` declares:

```jsonc
"name": "@njbrown/texturelab-runtime",
"version": "0.1.0",
"repository": "github:njbrown/texturelab",            // links the package to this repo on GitHub
"publishConfig": { "registry": "https://npm.pkg.github.com" },
"files": ["dist", "README.md", "LICENSE", "CHANGELOG.md"]
```

**Publish script: `gh-publish-runtime.sh`** (repo root, alongside `gh-release.sh`):

```bash
#!/usr/bin/env bash
#
# Builds web/packages/runtime and publishes it to GitHub Packages
# (npm.pkg.github.com) as @njbrown/texturelab-runtime.
#
#   ./gh-publish-runtime.sh            # publish the version in package.json
#   ./gh-publish-runtime.sh --dry-run  # build + pack, show what would be published
#
# Bump "version" in web/packages/runtime/package.json (and CHANGELOG.md) first.
# Needs a gh login with write:packages:  gh auth refresh -s write:packages
set -euo pipefail

if [ -f .env ]; then source .env; fi
# Use the keyring login; the exported GH_TOKEN only has read:packages.
unset GH_TOKEN GITHUB_TOKEN

PKG_DIR="web/packages/runtime"
DRY_RUN=false
[ "${1:-}" = "--dry-run" ] && DRY_RUN=true

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
    echo "Refusing to publish: uncommitted changes to tracked files." >&2
    exit 1
fi

NAME=$(node -p "require('./$PKG_DIR/package.json').name")
VERSION=$(node -p "require('./$PKG_DIR/package.json').version")
echo "Publishing $NAME@$VERSION from $(git rev-parse --short HEAD)"

# Version must be new — the registry rejects republishing a version anyway,
# but failing here is faster and clearer.
if npm view "$NAME@$VERSION" version --registry=https://npm.pkg.github.com \
        --//npm.pkg.github.com/:_authToken="$(gh auth token)" >/dev/null 2>&1; then
    echo "$NAME@$VERSION is already published — bump the version." >&2
    exit 1
fi

yarn --cwd web install --frozen-lockfile
yarn --cwd web check-drift
yarn --cwd web workspace "$NAME" test
yarn --cwd web workspace "$NAME" build

cd "$PKG_DIR"
if $DRY_RUN; then
    npm pack --dry-run
else
    npm publish --registry=https://npm.pkg.github.com \
        --//npm.pkg.github.com/:_authToken="$(gh auth token)"
    echo "Published $NAME@$VERSION"
fi
```

- It needs only Node and Yarn, not Qt or a C++ build. The image parity harness stays a separate CI workflow.
- It's run by hand, like `gh-release.sh`. It could later run as a `workflow_dispatch` job using the built-in `GITHUB_TOKEN` (`permissions: packages: write`), with the script reused as is.
- No git tag is needed: each published version records the commit it was built from via `gitHead` in the package metadata.

**Consuming it from the website repo:**

```ini
# website/.npmrc
@njbrown:registry=https://npm.pkg.github.com
//npm.pkg.github.com/:_authToken=${NODE_AUTH_TOKEN}
```
```jsonc
"dependencies": { "@njbrown/texturelab-runtime": "^0.1.0" }
```

**Caveat: GitHub Packages' npm registry requires authentication even for public packages.**
- Locally, any token with `read:packages` works. The `GH_TOKEN` already exported in your shell has that scope.
- The website's CI and hosting build (Vercel / Cloudflare Pages etc.) need a `NODE_AUTH_TOKEN` env var: a classic PAT with only `read:packages`.
- In GitHub Actions within the same account, `GITHUB_TOKEN` works if the website repo is granted read access in the package settings.
- If that ever becomes annoying, the same script can publish to npmjs.com instead by changing the registry and scope. Nothing else in the plan changes.

**Package shape:**
- **ESM only**, with `.d.ts` types and source maps. Target ES2020, with no Node built-ins in browser code.
- **Subpath exports**, so the website can pull the core without three.js (e.g. for the render worker):
  ```jsonc
  "exports": {
    ".":        { "types": "./dist/core/index.d.ts",   "import": "./dist/core/index.js" },
    "./viewer": { "types": "./dist/viewer/index.d.ts", "import": "./dist/viewer/index.js" },
    "./worker": { "import": "./dist/worker/index.js" }
  },
  "peerDependencies": { "three": ">=0.160" },       // viewer only; optional peer
  "sideEffects": false
  ```
- **Shaders live inside the node classes as template strings**, as in Electron, so the package never fetches its own files at runtime. That avoids path problems across Vite, webpack, Next and SvelteKit.
- **Workers:** shipped as module workers created with `new Worker(new URL('./worker.js', import.meta.url), { type: 'module' })`, which Vite and webpack 5 resolve automatically. Also accept a worker factory option in case a host bundler doesn't.
- **No binary assets in the package.** Meshes are generated from three.js geometries, and lighting defaults to a generated sky. HDRIs are passed in as URLs (`environment: { type: 'hdr', url }`), so the website serves them from its own CDN. The package stays small and makes no network requests unless the host asks for an HDRI.
- **SSR-safe:** importing the package on the server must not touch `window` or `document`. GPU work starts only when `createRuntime()` / `createViewer()` is called. This matters for SvelteKit/Next pages that render on the server.
- **Versioning:** semver on the runtime API. The supported `libraryVersion`s and node type names (`supportedNodes('v3')`) are exposed as exports, so the website knows which `.texture` files a given runtime can render. Changelog in `web/packages/runtime/CHANGELOG.md`.

**Public API (sketch):**

```ts
import { createRuntime, parseTexture } from '@njbrown/texturelab-runtime';
import { createViewer } from '@njbrown/texturelab-runtime/viewer';

const doc = parseTexture(jsonText);               // throws TextureParseError w/ unsupported nodes list
const runtime = createRuntime({ canvas, maxResolution: 1024 });
const result = await runtime.evaluate(doc, { onProgress: (done, total) => {} });
// result.channels: { albedo?, normal?, roughness?, metalness?, height?, ao?, alpha? }
const viewer = createViewer(container, { mesh: 'sphere' });         // default: procedural gradient sky
viewer.setEnvironment({ type: 'hdr', url: 'https://cdn.example.com/env/studio_1k.hdr' }); // optional
viewer.setMaterial(result.channels);
await result.exportPNGs({ resolution: 2048 });   // Blob per channel, for downloads / render worker
```

**Local development across both repos:** while working on the runtime and website together, use `yarn link` (or `file:../texturelab/web/packages/runtime`) in the website, and switch back to the published version before committing.

---

## 3. Runtime design

### 3.1 TS node library

Each node is a TS class mirroring its C++ counterpart, the same way the Electron version worked:

```ts
// web/packages/runtime/src/library/v3/normalmapv3.ts  ↔  src/texturelab/libraries/v3/normalmapv3.cpp
export class NormalMapV3Node extends GpuNode {
  init() {
    this.title = 'Normal Map';
    this.addInput('height');
    this.addFloatProp('strength', 'Strength', 1, 0, 8, 0.01);
    this.addEnumProp('convention', 'Convention', ['OpenGL', 'DirectX']);
    this.setShaderSource(/* glsl */ `
      vec4 process(vec2 uv) { ... }   // copied verbatim from the .cpp raw string
    `);
  }
}
```

- **API parity with C++:** `addInput`, `addFloatProp` / `addIntProp` / `addBoolProp` / `addColorProp` / `addEnumProp` / `addGradientProp` / `addCurveProp` / `addImageProp` / `addStringProp`, `setShaderSource`, property groups. With matching names and argument order, a port is mostly copy-paste.
- **Multi-pass nodes** (autolevels, blurhq, fastao, distancetransform, bevelv2, spread, floodfillv2) override `render(ctx)` instead of setting a single shader. They mirror the C++ `NodeTextureRenderer` subclasses.
- **Shader assembly** mirrors `buildShaderForNode` in `texturerenderer.cpp`: the preamble, then `randomLib` / `gradientLib` / `curveLib`, then `uniform sampler2D x` + `uniform bool x_connected` per input, then `prop_*` uniforms, then the node body. The prelude is ported once into `library/prelude.ts`.
- **GLSL ES:** node bodies stay unchanged. Only the prelude differs (`#version 300 es`, `precision highp float; precision highp int; precision highp sampler2D;`). If a node body uses something ES 3.00 lacks, fix it in **both** C++ and TS so the bodies stay byte-comparable.
- **Registry:** `library/v2/index.ts` and `library/v3/index.ts` map `typeName` → class, matching `createLibraryV2()` / `createLibraryV3()` in `library.cpp`, including the v3 = v2 − removed + added composition.

**Porting workflow:**
1. **v1/v2:** restore from git (`git show f62fa83^:src/lib/library/v2/<node>.ts`). Rename to the new base-class API and diff against the current C++ to pick up fixes made since 2022.
2. **v3:** `scripts/port-node.ts` scaffolds a TS file from each `.cpp`. It extracts the `R""""(...)""""` shader body and turns `add*Prop(...)` calls into the TS form. A person then reviews and finishes it, which is where the multi-pass nodes need real work.
3. Every node gets a per-node render test using its demo file in `public/assets/nodes/<node>.texture`.

**Drift check (`scripts/check-drift.ts`, runs in CI on every PR):** a plain Node script, with no Qt. For each node it extracts the GLSL body and the prop declarations (name, type, default, range, enum options) from both the `.cpp` and the `.ts`. It normalises whitespace and comments, then fails if they differ or if a node exists in one library and not the other. So a C++ shader change without the matching TS change fails CI immediately, before the slower image parity harness. Intentional differences are listed in `drift-allowlist.json` with a reason.

### 3.2 Evaluation

1. **Parse and validate** (zod schema). Coerce numeric strings. Reject unknown `typeName` with a clear message ("node `foo` not supported in web runtime yet"). Enforce limits on node count, embedded image size and resolution.
2. **Resolve library** from `libraryVersion` (v1 → v2 library, as on desktop).
3. **Topologically sort** from the channel output nodes backwards, and evaluate only nodes that reach an output.
4. **Per node:** compile (cached per typeName) → bind input textures (or a 1×1 black texture + `x_connected=false`), props as uniforms (gradients/curves as struct arrays, colors as vec4, enums as int), `_seed`, `_textureSize` → draw a fullscreen triangle into the node's render target.
5. **Multi-pass nodes** override `render(ctx)` (§3.1). `floodfillv2` runs its BFS on the CPU (in a Worker) after a `readPixels`, then uploads the result.
6. **Outputs:** the textures for `editor.textureChannels` (albedo, normal, roughness, metalness, height, ao, alpha) are copied to **RGBA8** (RGBA16F for height) viewer textures with mipmaps, then handed to the viewer.

### 3.3 GPU memory and formats

- Render targets: RGBA32F if `EXT_color_buffer_float` is available (all desktop browsers), otherwise RGBA16F (`EXT_color_buffer_half_float`), otherwise RGBA8 with a "reduced precision" warning (some mobile GPUs).
- **Free each intermediate texture as soon as its last consumer has rendered** (refcounted pool keyed by size and format). Peak memory then scales with graph width instead of node count.
- Default preview resolution: **1024**. Allow 2048 on desktop, and cap at 512–1024 on mobile, ignoring the file's `textureWidth` when it's larger.
- Handle `webglcontextlost` by re-running the evaluation, since all state can be rebuilt from the file.

### 3.4 Responsiveness

- Phase 1 runs on the main thread, with each node pass in its own `requestAnimationFrame`, so big graphs show progress instead of freezing the page.
- Phase 2 moves the runtime into a **Worker with OffscreenCanvas**, which shares textures with the viewer through `ImageBitmap` transfer, or runs the viewer in the same worker.
- Progressive mode: evaluate at 256 first, display it, then re-evaluate at full resolution.

### 3.5 Viewer

- three.js: `MeshStandardMaterial` (albedo map, normal map in OpenGL convention, roughness, metalness, aoMap, and displacement from height on a high-subdivision sphere with a scale slider), plus image-based lighting through `PMREMGenerator`.
- **Environments (nothing bundled):**
  ```ts
  type Environment =
    | { type: 'gradient'; zenith?: Color; horizon?: Color; ground?: Color;      // default
        sun?: { direction: Vec3; color?: Color; intensity?: number; size?: number } | false;
        background?: 'sky' | 'blurred' | Color }
    | { type: 'studio' }                        // three.js RoomEnvironment: generated softboxes
    | { type: 'hdr' | 'exr'; url: string; rotation?: number; intensity?: number; background?: 'sky' | 'blurred' | Color };
  ```
  - **`gradient`** (default): a shader draws a zenith → horizon → ground gradient, plus a soft sun disc, onto a sky sphere. `PMREMGenerator.fromScene` turns it into the lighting environment. The sun disc matters: without a bright spot, metals and glossy surfaces look flat. This takes a few milliseconds, needs no network, and gives the same result every time, which keeps server thumbnails consistent.
  - **`studio`**: three.js's `RoomEnvironment` (from `three/examples/jsm`), also generated with no asset files. It gives neutral softbox highlights and is a good default for thumbnails.
  - **`hdr` / `exr`**: loaded at runtime with `RGBELoader` / `EXRLoader` from a host-supplied URL. If loading fails, the viewer falls back to the gradient sky and emits an error event.
  - Presets (e.g. `'overcast'`, `'sunset'`, `'noon'`) are just named gradient settings, so they cost nothing.
- **The playground** serves the existing `public/assets/env/*_1k.hdr` through its own dev server to test the URL path. They're never copied into the package.
- Normal maps: three.js expects OpenGL (+Y). `normalmapv3` has a `convention` prop, so flip the green channel in the material when a file uses the DirectX convention.
- Albedo is sRGB and data maps are linear. Set `colorSpace` correctly, since that's the usual cause of washed-out previews.
- Phase 1 has an orbit camera and a sphere. Phase 2 adds the cube, plane and cylinder meshes, an environment picker (sky presets + HDRI URL list), a tiling slider and a 2D channel view.

---

## 4. Phases

### Phase 0 — Spikes (≈ 2–3 days)
- [ ] Restore the Electron v1/v2 node files from `f62fa83^` and count how many still match the current C++ shader bodies, to size the sync work.
- [ ] Port 5 representative nodes by hand (noise, gradient-using, curve-using, blend, normalmapv3). Check that the ES prelude plus the unchanged C++ bodies compile in Chrome, Firefox and Safari.
- [ ] Prototype `check-drift.ts` against those 5 nodes.
- [ ] Check that RGBA32F render-to-texture works in Chrome, Firefox and Safari (desktop), and see what iOS Safari supports.

**Exit:** the TS node API is settled and the port effort for the remaining nodes is estimated.

### Phase 1 — Drag & drop playground (≈ 1–2 weeks)
- [ ] `web/` Yarn workspace, Vite + TypeScript, ESLint/Prettier, Vitest.
- [ ] `@njbrown/texturelab-runtime` package skeleton with the final `exports` map, a library-mode build (Vite lib or tsup) and `.d.ts` output. The playground imports it **only through its public entry points**, never by deep paths, so it behaves exactly like the website will.
- [ ] Runtime: parser + schema, scheduler, WebGL2 backend, shader prelude, node base class.
- [ ] Port all single-pass v3 nodes, plus the v2 nodes that v3 inherits, to TS. Use `port-node.ts` for v3 and the restored Electron files for v2.
- [ ] `check-drift.ts` running in CI.
- [ ] Viewer (`/viewer` subpath): sphere, gradient-sky environment, an `hdr` URL loader, and orbit controls.
- [ ] Playground page: a drop zone, a file picker fallback, a per-node progress indicator, and an error panel that lists any unsupported nodes.
- [ ] `gh-publish-runtime.sh`: publish `0.1.0` to GitHub Packages. Smoke test: a throwaway Vite app outside the repo, with the `.npmrc` above, installs `@njbrown/texturelab-runtime@0.1.0` and renders an example.

**Exit:** the examples that don't use custom-renderer nodes render correctly on the sphere. The rest fail cleanly and name the missing node. A separate repo can install the package from GitHub Packages and render a file.

### Phase 2 — Full parity and polish (≈ 2–3 weeks)
- [ ] Port the multi-pass nodes: autolevels, blurhq, fastao, distancetransform, bevelv2, spread, floodfillv2 (+ v2 bevel/floodfill).
- [ ] Complete the v2 library (nodes removed in v3), so legacy files render with their original library.
- [ ] **Parity harness:** desktop gets a headless `--export` CLI that renders `public/assets/examples` and `public/assets/nodes` to PNG. Playwright renders the same files with the web runtime. CI compares them per channel against a tolerance threshold and fails on regressions.
- [ ] Worker/OffscreenCanvas, progressive rendering, resolution selector, mesh and environment switcher, channel view.
- [ ] Download the outputs as a zip of PNGs, using the export naming from `export.filePattern`.

**Exit:** all 10 examples and ≥ 95 % of the node demo files pass the parity test.

### Phase 3 — Gallery MVP (≈ 4–6 weeks)
See §5.

### Phase 4 — Later
- Likes, collections, follows and comments.
- **In-browser parameter tweaking:** authors mark props as "exposed" and visitors get sliders. The runtime re-evaluates dirty nodes only, which is the same dirty propagation as desktop.
- **Desktop integration:** "Publish to gallery" from TextureLab, and browsing and importing gallery textures from the launcher (`src/catalog` already has an index and thumbnail cache).
- Version history per texture, a public API, and an embeddable viewer widget.

---

## 5. Gallery design (plan now, build in phase 3)

The gallery is built in the **website repo** and consumes `@njbrown/texturelab-runtime` from GitHub Packages at a pinned version (§2.1). Upgrading the runtime is a version bump in the website's `package.json`. The render worker uses the same pinned version as the browser, so thumbnails and the live viewer always match.

### 5.1 Stack (recommended)

| Concern | Choice | Why |
|---|---|---|
| Web app | SvelteKit (or Next.js if you prefer React) | SSR for SEO on the browse and texture pages, and simple API routes. The runtime package doesn't care which. |
| DB | Postgres (Neon / Supabase / RDS) | Relational data (users, textures, tags, likes) plus full-text search via `tsvector` to start. |
| Auth | A managed or library solution (Auth.js / Lucia / Supabase Auth): email + GitHub/Google/Discord OAuth | Discord fits the existing community. Don't build password handling yourself. |
| Files | S3-compatible object storage (Cloudflare R2: no egress fees) + CDN | Downloads of maps and thumbnails are the main bandwidth cost. |
| Jobs | A queue (Postgres-backed, e.g. pg-boss/graphile-worker, to start) | Render jobs and moderation tasks. |
| Render worker | Node + Playwright + headless Chromium running **the same web runtime** | Thumbnails then match exactly what visitors see, with one code path. GPU instance if volume needs it, SwiftShader (CPU) at first. |

### 5.2 Upload pipeline

1. The client gives an **instant local preview**: the runtime renders the file in the uploader's browser, which shows unsupported nodes before upload. The user fills in title, description, tags and license.
2. The server checks size (e.g. ≤ 20 MB), the JSON schema, node types against the runtime's `supportedNodes()` allowlist, resolution limits, and embedded images. It re-encodes or strips them if they're malformed, and checks pixel dimensions to block decompression bombs.
3. The `.texture` is stored unchanged in object storage, keyed by content hash (which also gives dedupe).
4. A render job runs: the headless runtime produces **server-trusted** outputs. These are a sphere thumbnail (512, webp), a flat tile preview, and the channel maps at 1K/2K (PNG, plus 16-bit height) for download.
5. The texture is published, or sent to a moderation queue for new accounts.

Client-rendered thumbnails are never trusted for the public listing.

### 5.3 Data model (initial)

```
users(id, handle, display_name, avatar_url, created_at, role, banned_at)
textures(id, owner_id, slug, title, description, license, status[draft|pending|published|removed],
         current_version_id, library_version, created_at, updated_at, download_count, like_count)
texture_versions(id, texture_id, file_key, file_sha256, node_count, width, height, created_at)
renders(id, version_id, kind[thumb|tile|map], channel, resolution, format, file_key)
tags(id, name)            texture_tags(texture_id, tag_id)
likes(user_id, texture_id, created_at)
reports(id, texture_id, reporter_id, reason, status, created_at)
```

### 5.4 Pages
- **Browse:** grid of sphere thumbnails, filters (tags, category, license, newest/popular) and search.
- **Texture page:** a live 3D viewer (the runtime renders the real graph, so it's interactive). Thumbnails are a placeholder until it's ready. It also shows the author, license, tags and download buttons for the `.texture` and the maps zip at each resolution.
- **Profile:** the user's uploads and likes.
- **Upload / edit**, plus an admin moderation queue.

### 5.5 Non-functional
- **Licensing of uploads:** pick a default (CC0 like ambientCG, or a CC-BY choice per upload), and add an upload ToS checkbox.
- **GPL:** the web runtime ships shader code derived from GPLv3 C++ to browsers, which counts as distribution. Keep `web/` GPLv3 and open source, which is consistent with the repo. User-uploaded `.texture` files are user content, not covered by the GPL. Writing this into the ToS is worth it.
- **Abuse:** rate-limit uploads, require email verification, keep the reports queue, and limit embedded image size, since embedded images are the only arbitrary user pixels in a file.
- **Cost:** most of it is storage and bandwidth for exported maps. Generate 2K/4K maps lazily on first download and cache them.

---

## 6. Risks

| Risk | Mitigation |
|---|---|
| Two copies of every node (C++ and TS) drift apart | A source-level drift check on every PR, the image parity harness, and a contributor note: "a node change touches both `.cpp` and `.ts`". |
| Porting effort for about 120 node classes across v2 + v3 | Most of v2 comes back from Electron history, v3 is scaffolded by `port-node.ts`, and only about 9 multi-pass nodes need real engineering. |
| Float render targets missing on some mobile GPUs | Fall back to half float / RGBA8 and cap resolution. Lower quality on mobile is acceptable for previews. |
| VRAM blow-up on big graphs at 2K+ | Release intermediates with refcounting, resolution caps, and context-loss recovery. |
| GitHub Packages needs an auth token even for public packages, which can break website CI or hosting builds | Document the `.npmrc` + `NODE_AUTH_TOKEN` setup in the runtime README. The same script can switch to npmjs.com if it becomes a problem. |
| Bundler or SSR incompatibility in the website | ESM-only, no runtime asset fetching, SSR-safe imports, and a worker-factory escape hatch. Tested from a fresh Vite app, and from a SvelteKit/Next app once the website's framework is chosen. |
| `floodfillv2` CPU path is slow at 2K | Run it in a Worker. Use a GPU connected-components (label propagation) version later if needed. |
| Headless-Chromium render worker throughput | SwiftShader first, measure, move to a GPU instance when volume justifies it. |

## 7. Open questions
1. Default license for gallery uploads (CC0 vs. a per-upload choice)?
2. Should visitors be able to tweak exposed parameters in phase 3, or only from phase 4?
3. Hosting and budget preference (Cloudflare / Vercel / self-hosted VPS)? This affects the render-worker choice.
4. Gallery domain and branding: part of the TextureLab site, or separate?
5. Is a desktop "publish to gallery" button needed for launch, or is web upload enough?
