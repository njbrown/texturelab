# @njbrown/texturelab-runtime

Evaluates [TextureLab](https://github.com/njbrown/texturelab) `.texture` graphs in the browser with WebGL2, and previews the result as a PBR material with three.js.

## Install

The package is published to GitHub Packages. Add this to your project's `.npmrc`:

```ini
@njbrown:registry=https://npm.pkg.github.com
//npm.pkg.github.com/:_authToken=${NODE_AUTH_TOKEN}
```

GitHub Packages requires a token for installs, even for public packages. Locally, any token with `read:packages` works. In CI or a hosting build, set `NODE_AUTH_TOKEN`.

```sh
yarn add @njbrown/texturelab-runtime three
yarn add -D @types/three   # for the viewer's types
```

`three` is only needed if you use the `/viewer` entry point.

## Usage

```ts
import { createRuntime, parseTexture } from "@njbrown/texturelab-runtime";
import { createViewer } from "@njbrown/texturelab-runtime/viewer";

const runtime = createRuntime();                 // WebGL2 on an internal OffscreenCanvas
const viewer = createViewer(document.getElementById("preview")!);

const doc = parseTexture(await file.text());     // throws TextureParseError
const result = await runtime.evaluate(doc, {
    resolution: 1024,
    onProgress: ({ done, total }) => console.log(`${done}/${total}`),
});

for (const w of result.warnings) console.warn(w.message);   // unsupported nodes, shader errors, ...
viewer.setMaps(result.toMaps());
result.dispose();
```

### Texture variables

A `.texture` file can expose variables, which are named values that drive node props, created in the desktop app's **Variables** tab. They're parsed into `doc.variables` (in display order) with their type, bounds, enum options and current value:

```ts
import { setVariable, resetVariables } from "@njbrown/texturelab-runtime";

for (const v of doc.variables) console.log(v.name, v.type, v.value, v.min, v.max, v.options);

setVariable(doc, "roughness", 0.4);   // clamped to the variable's bounds / enabled options
const result = await runtime.evaluate(doc);   // re-render to see it
resetVariables(doc);                  // back to the values in the file
```

The types are `float`, `int`, `bool`, `enum` and `color`. An enum's value is an index into the linked props' option list, and `options` lists the ones to offer, with their labels.

### Viewer

- **Meshes:** `viewer.setMesh("sphere" | "cube" | "plane" | "cylinder")`.
- **Lighting:** nothing is bundled. The default is a generated gradient sky. Use `SKY_PRESETS` for variants, `{ type: "studio" }` for three.js's generated room, or `{ type: "hdr" | "exr", url }` for an HDRI you serve yourself:
  ```ts
  viewer.setEnvironment({ type: "hdr", url: "https://cdn.example.com/env/studio_1k.hdr" });
  ```
- **Other settings:** `setDisplacementScale`, `setNormalConvention("opengl" | "directx")`, `setTiling`, `setExposure` and `setBackground`.

## Status

- **Node coverage:** every node in the v1/v2/v3 libraries is ported.
  - Multi-pass nodes (autolevels, blurhq, fastao, distancetransform, bevelv2, spread) use the same passes and GLSL as the desktop renderers.
  - CPU nodes (floodfillv2, and v2's bevel/floodfill) run the same algorithms in JS on read-back pixels. They take about 1 s at 2048 and currently run on the main thread.
- **Rendering:** files render with the library version they were saved with. They are never migrated.

## Development

Node classes in `src/core/libraries/` are ported from `src/texturelab/libraries/` in this repo. When a C++ node changes, change its `.ts` twin too.

- **Single-pass nodes** are scaffolded by `scripts/port-node.ts`.
- **Multi-pass and CPU nodes** are hand ports against `src/core/gl/noderenderer.ts`, which mirrors `graphics/noderenderer.h`. They implement `createRenderer()` / `createRenderData()` or `cpuProcess()`.
- **The registry** (`registry.ts`) is generated from `library.cpp`.

```sh
yarn --cwd web port-nodes        # scaffold .ts for new C++ nodes + regenerate registry.ts
```

Publish with `./gh-publish-runtime.sh` from the repo root.
