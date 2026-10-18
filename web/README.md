# TextureLab web

This is the browser runtime for `.texture` files. It's a Yarn (1.x) workspace with these parts:

| Path | What |
|---|---|
| `packages/runtime` | `@njbrown/texturelab-runtime`: graph evaluation (WebGL2) and the three.js viewer. This is the package the website installs. |
| `apps/playground` | A drag-and-drop page: drop a `.texture` file and see it on a sphere. |
| `scripts/port-node.ts` | Scaffolds TS node classes from the C++ libraries and regenerates the library registry. |
| `tests/smoke.ts` | Renders files in headless Chrome and reports shader errors and unsupported nodes. |

The plan is in [`docs/web-runtime-plan.md`](../docs/web-runtime-plan.md).

## Getting started

Needs Node 24+ and Yarn 1.22.

```sh
cd web
yarn install
yarn dev                    # playground on http://localhost:5199
```

The playground resolves the runtime package to its TS sources, so runtime edits hot-reload. It also serves `../public/assets`, which provides the example graphs, the node demo files and the HDRIs. None of those go into the package.

## Checks

```sh
yarn typecheck
yarn smoke                          # the 10 example graphs
node tests/smoke.ts --nodes         # + per-node demo files
node tests/smoke.ts --all-nodes     # compile + render every node type in v2 and v3
```

The smoke test uses your installed Chrome. Set `CHROME_PATH` if it can't find it. Screenshots go to `tests/out/`.
