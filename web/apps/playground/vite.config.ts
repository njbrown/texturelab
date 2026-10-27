import { readdirSync, statSync } from "node:fs";
import { join, relative, resolve } from "node:path";
import { defaultClientConditions, defineConfig } from "vite";

// The playground serves the repo's existing assets (example .texture files
// and HDRIs) from public/assets. They're dev/test fixtures only — nothing
// here is copied into the runtime package.
const assets = resolve(__dirname, "../../../public/assets");

function findFiles(dir: string, ext: string): string[] {
    const out: string[] = [];
    for (const name of readdirSync(dir)) {
        const path = join(dir, name);
        if (statSync(path).isDirectory()) out.push(...findFiles(path, ext));
        else if (name.endsWith(ext)) out.push(relative(assets, path));
    }
    return out.sort();
}

export default defineConfig({
    publicDir: assets,
    resolve: {
        // Resolve @njbrown/texturelab-runtime to its TS sources through the
        // package's own export map, so edits hot-reload without a build.
        conditions: ["texturelab-source", ...defaultClientConditions],
    },
    define: {
        __EXAMPLES__: JSON.stringify(findFiles(join(assets, "examples"), ".texture")),
        __NODE_DEMOS__: JSON.stringify(findFiles(join(assets, "nodes"), ".texture")),
        __HDRIS__: JSON.stringify(findFiles(join(assets, "env"), ".hdr")),
    },
    server: { port: 5199 },
    preview: { port: 5199 },
});
