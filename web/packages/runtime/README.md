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

### Viewer

- **Meshes:** `viewer.setMesh("sphere" | "cube" | "plane" | "cylinder")`.
- **Lighting:** nothing is bundled. The default is a generated gradient sky. Use `SKY_PRESETS` for variants, `{ type: "studio" }` for three.js's generated room, or `{ type: "hdr" | "exr", url }` for an HDRI you serve yourself:
  ```ts
  viewer.setEnvironment({ type: "hdr", url: "https://cdn.example.com/env/studio_1k.hdr" });
  ```
- **Other settings:** `setDisplacementScale`, `setNormalConvention("opengl" | "directx")`, `setTiling`, `setExposure` and `setBackground`.

## Status

- **Node coverage:** every single-pass node in the v1/v2/v3 libraries is ported. The multi-pass nodes are not ported yet: autolevels, blurhq, fastao, distancetransform, bevelv2, spread, floodfillv2, and v2's bevel/floodfill. They render black and are reported in `result.warnings`.
- **Rendering:** files render with the library version they were saved with. They are never migrated.

## Development

Node classes in `src/core/libraries/` are ported from `src/texturelab/libraries/` in this repo. When a C++ node changes, change its `.ts` twin too. The registry (`registry.ts`) is generated from `library.cpp`:

```sh
yarn --cwd web port-nodes        # scaffold .ts for new C++ nodes + regenerate registry.ts
```

Publish with `./gh-publish-runtime.sh` from the repo root.
