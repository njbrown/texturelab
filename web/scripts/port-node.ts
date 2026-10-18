// Scaffolds TS node classes from the C++ node libraries.
//
//   node scripts/port-node.ts            # port every node that has no .ts yet
//   node scripts/port-node.ts --force    # overwrite existing .ts files too
//   node scripts/port-node.ts v3/swirl   # port one file (relative to libraries/)
//
// For each src/texturelab/libraries/vN/<file>.cpp it writes
// packages/runtime/src/core/libraries/vN/<file>.ts with one class per
// `void X::init()` found in the file. The C++ init() bodies are a small,
// regular vocabulary (title, addInput, add*Prop, createGroup, setShaderSource)
// so the translation is mechanical: GLSL is copied verbatim, C++ syntax is
// rewritten to TS. Anything it doesn't recognise is emitted as a
// `// TODO(port):` comment and reported, for a person to finish.
//
// The library registries (libv2.ts / libv3.ts) are always regenerated from
// createLibraryV2() / createLibraryV3() in libraries/library.cpp.
//
// Generated files are meant to be edited afterwards; re-running without
// --force never touches a file that already exists.

import { existsSync, mkdirSync, readdirSync, readFileSync, writeFileSync } from "node:fs";
import { basename, dirname, join, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const repoRoot = resolve(here, "../..");
const cppRoot = join(repoRoot, "src/texturelab/libraries");
const tsRoot = join(repoRoot, "web/packages/runtime/src/core/libraries");

const args = process.argv.slice(2);
const force = args.includes("--force");
const only = args.filter((a) => !a.startsWith("--"));

const warnings: string[] = [];

// ---------------------------------------------------------------------------
// C++ source helpers
// ---------------------------------------------------------------------------

interface RawStrings {
    text: string;
    raws: string[];
}

/** Replace every C++ raw string literal R"delim(...)delim" with @@RAWn@@. */
function extractRawStrings(src: string): RawStrings {
    const raws: string[] = [];
    const text = src.replace(/R"([^(\s]*)\(([\s\S]*?)\)\1"/g, (_m, _delim, body: string) => {
        raws.push(body);
        return `@@RAW${raws.length - 1}@@`;
    });
    return { text, raws };
}

function stripComments(src: string): string {
    // Raw strings are already placeholders, but ordinary "..." strings could
    // contain // so skip over them.
    let out = "";
    for (let i = 0; i < src.length; i++) {
        const c = src[i]!;
        if (c === '"') {
            let j = i + 1;
            while (j < src.length && src[j] !== '"') j += src[j] === "\\" ? 2 : 1;
            out += src.slice(i, j + 1);
            i = j;
        } else if (c === "/" && src[i + 1] === "/") {
            while (i < src.length && src[i] !== "\n") i++;
            out += "\n";
        } else if (c === "/" && src[i + 1] === "*") {
            const end = src.indexOf("*/", i + 2);
            i = end < 0 ? src.length : end + 1;
        } else {
            out += c;
        }
    }
    return out;
}

/** Body of the brace block starting at src[open] === "{". */
function blockBody(src: string, open: number): string {
    let depth = 0;
    for (let i = open; i < src.length; i++) {
        if (src[i] === "{") depth++;
        else if (src[i] === "}" && --depth === 0) return src.slice(open + 1, i);
    }
    throw new Error("unbalanced braces");
}

function splitStatements(body: string): string[] {
    const out: string[] = [];
    let cur = "";
    let depth = 0;
    let inStr = false;
    for (let i = 0; i < body.length; i++) {
        const c = body[i]!;
        cur += c;
        if (inStr) {
            if (c === "\\") cur += body[++i] ?? "";
            else if (c === '"') inStr = false;
            continue;
        }
        if (c === '"') inStr = true;
        else if ("({[".includes(c)) depth++;
        else if (")}]".includes(c)) depth--;
        else if (c === ";" && depth === 0) {
            out.push(cur.trim());
            cur = "";
        }
    }
    if (cur.trim()) out.push(cur.trim());
    return out;
}

function toTemplateLiteral(glsl: string): string {
    const escaped = glsl.replace(/\\/g, "\\\\").replace(/`/g, "\\`").replace(/\$\{/g, "\\${");
    return "/* glsl */ `" + escaped + "`";
}

// ---------------------------------------------------------------------------
// Statement translation
// ---------------------------------------------------------------------------

/** Rewrite C++ expression syntax to TS outside of string literals. */
function rewriteExpr(expr: string): string {
    let out = "";
    for (let i = 0; i < expr.length; i++) {
        const c = expr[i]!;
        if (c === '"') {
            let j = i + 1;
            while (j < expr.length && expr[j] !== '"') j += expr[j] === "\\" ? 2 : 1;
            out += expr.slice(i, j + 1);
            i = j;
            continue;
        }
        out += c;
    }
    // Apply token rewrites on the non-string parts only.
    return out
        .split(/("(?:[^"\\]|\\.)*")/)
        .map((part, idx) => {
            if (idx % 2 === 1) return part; // string literal
            return part
                .replace(/\bthis->/g, "this.")
                .replace(/->/g, ".")
                .replace(/\bQList<QString>\s*\{/g, "{")
                .replace(/\bQStringList\s*\{/g, "{")
                .replace(/\{/g, "[")
                .replace(/\}/g, "]")
                .replace(/::/g, ".")
                .replace(/(\d+\.\d*|\.\d+|\d+)[fF]\b/g, "$1")
                .replace(/\s+/g, " ");
        })
        .join("");
}

const KNOWN_CALL =
    /^(?:this\.)?(?:\w+\.add\()?\s*this\.(addInput|addIntProp|addFloatProp|addBoolProp|addEnumProp|addColorProp|addStringProp|addGradientProp|addImageProp|addCurveProp)\s*\(/;

function translateStatement(stmt: string, raws: string[], where: string, reassigned: Set<string>): string {
    // Placeholders that stand alone become template literals.
    const withRaw = (s: string) => s.replace(/@@RAW(\d+)@@/g, (_m, n) => toTemplateLiteral(raws[Number(n)]!));

    let s = stmt.replace(/;$/, "").trim();

    // QColor white(255, 255, 255);
    let m = s.match(/^QColor\s+(\w+)\s*\((.*)\)$/s);
    if (m) return `const ${m[1]} = QColor(${rewriteExpr(m[2]!)});`;

    // auto x = <expr>;   (also QString/Gradient locals)
    m = s.match(/^(?:auto|QString|Gradient|const\s+auto)\s*[*&]?\s*(\w+)\s*=\s*(.*)$/s);
    if (m) {
        const rhs = m[2]!.trim();
        const decl = reassigned.has(m[1]!) ? "let" : "const";
        if (/^@@RAW\d+@@$/.test(rhs)) return `${decl} ${m[1]} = ${withRaw(rhs)};`;
        return `${decl} ${m[1]} = ${rewriteExpr(rhs)};`;
    }

    // prop = this->addEnumProp(...)   (reassigning an earlier local)
    m = s.match(/^(\w+)\s*=\s*(this->add\w+Prop\s*\(.*)$/s);
    if (m && reassigned.has(m[1]!)) return `${m[1]} = ${rewriteExpr(m[2]!)};`;

    // this->imageProp = this->addImageProp(...)  -> drop the member store
    m = s.match(/^this->\w+\s*=\s*(this->add\w+Prop\(.*)$/s);
    if (m) s = m[1]!;

    const ts = rewriteExpr(s);

    if (/^this\.title = /.test(ts)) return ts + ";";
    if (/^this\.usesCpuProcessing = /.test(ts)) return ts + ";";
    if (/^this\.setShaderSource\(/.test(ts)) return withRaw(ts) + ";";
    if (KNOWN_CALL.test(ts)) return ts + ";";
    if (/^\w+\.(index|collapsed) = /.test(ts)) return ts + ";";
    if (/^\w+\.setValue\(/.test(ts)) return ts + ";";
    if (/^\w+\.add\(this\.add\w+Prop\(/.test(ts)) return ts + ";";

    warnings.push(`${where}: untranslated statement: ${stmt.replace(/\s+/g, " ").slice(0, 120)}`);
    return `// TODO(port): ${stmt.replace(/\s+/g, " ")}`;
}

// ---------------------------------------------------------------------------
// File porting
// ---------------------------------------------------------------------------

interface PortedClass {
    className: string;
    file: string; // e.g. "v1/invert"
}

function portFile(version: string, cppFile: string): PortedClass[] {
    const rel = `${version}/${basename(cppFile, ".cpp")}`;
    const src = readFileSync(cppFile, "utf8");
    const { text: noRaw, raws } = extractRawStrings(src);
    const text = stripComments(noRaw);

    const classes: { name: string; body: string[] }[] = [];
    const usesHelpers = new Set<string>();

    for (const m of text.matchAll(/void\s+(\w+)::init\s*\(\s*\)\s*\{/g)) {
        const name = m[1]!;
        const open = m.index! + m[0].length - 1;
        const stmts = splitStatements(blockBody(text, open));
        const reassigned = new Set<string>();
        for (const st of stmts) {
            const r = st.match(/^(\w+)\s*=[^=]/);
            if (r && !["auto", "this"].includes(r[1]!)) reassigned.add(r[1]!);
        }
        const body = stmts.map((st) => translateStatement(st, raws, `${rel}.cpp ${name}`, reassigned));
        for (const line of body) {
            if (/\bQColor\(/.test(line)) usesHelpers.add("QColor");
            if (/\bGradient\./.test(line)) usesHelpers.add("Gradient");
        }
        classes.push({ name, body });
    }

    if (classes.length === 0) return [];

    // Nodes with a custom NodeTextureRenderer get the MultiPassNode base.
    const multiPass = /::createRenderer\s*\(/.test(text);
    const base = multiPass ? "MultiPassNode" : "TextureNode";

    const out = join(tsRoot, `${rel}.ts`);
    if (!existsSync(out) || force) {
        const imports = [`import { ${base} } from "../../node.js";`];
        if (usesHelpers.has("QColor")) imports.push(`import { QColor } from "../../color.js";`);
        if (usesHelpers.has("Gradient")) imports.push(`import { Gradient } from "../../gradient.js";`);

        const parts = [
            `// Ported from src/texturelab/libraries/${rel}.cpp by scripts/port-node.ts.`,
            `// Keep the GLSL and props in sync with the C++ when either changes.`,
            "",
            ...imports,
            "",
        ];
        if (multiPass) {
            parts.push(
                "// TODO(port): the C++ node renders with a custom multi-pass",
                "// NodeTextureRenderer, which isn't ported yet. init() is ported so files",
                "// containing this node load and report it as unsupported.",
                "",
            );
        }
        for (const cls of classes) {
            parts.push(`export class ${cls.name} extends ${base} {`);
            parts.push("    override init(): void {");
            for (const line of cls.body) {
                // Only the first line: the rest is GLSL inside a template literal,
                // copied verbatim.
                parts.push("        " + line);
            }
            parts.push("    }", "}", "");
        }
        mkdirSync(dirname(out), { recursive: true });
        writeFileSync(out, parts.join("\n"));
        console.log(`wrote ${relative(repoRoot, out)} (${classes.map((c) => c.name).join(", ")})`);
    }

    return classes.map((c) => ({ className: c.name, file: rel }));
}

// ---------------------------------------------------------------------------
// Library registries
// ---------------------------------------------------------------------------

function generateRegistries(classFiles: Map<string, string>): void {
    const src = stripComments(extractRawStrings(readFileSync(join(cppRoot, "library.cpp"), "utf8")).text);

    const fnBody = (name: string) => {
        const m = src.match(new RegExp(`Library\\*\\s+${name}\\s*\\(\\s*\\)\\s*\\{`));
        if (!m) throw new Error(`${name} not found in library.cpp`);
        return blockBody(src, m.index! + m[0].length - 1);
    };

    const used = new Set<string>();
    const translate = (body: string) => {
        const lines: string[] = [];
        for (const st of splitStatements(body)) {
            const s = st.replace(/\s+/g, " ");
            let m = s.match(/^lib->addNode<(\w+)>\(\s*"([^"]+)",\s*"([^"]*)"/);
            if (m) {
                const [, cls, name, display] = m;
                if (!classFiles.has(cls!)) throw new Error(`no ported class for ${cls}`);
                used.add(cls!);
                lines.push(`    lib.addNode(${JSON.stringify(name)}, ${JSON.stringify(display)}, ${cls});`);
                continue;
            }
            m = s.match(/^lib->items\.remove\("([^"]+)"\)/);
            if (m) {
                lines.push(`    lib.removeNode(${JSON.stringify(m[1])});`);
                continue;
            }
            if (/^(auto lib = |return lib)/.test(s)) continue;
            throw new Error(`library.cpp: unrecognised statement: ${s}`);
        }
        return lines;
    };

    const v2 = translate(fnBody("createLibraryV2"));
    const v3 = translate(fnBody("createLibraryV3"));

    const byFile = new Map<string, string[]>();
    for (const cls of [...used].sort()) {
        const file = classFiles.get(cls)!;
        byFile.set(file, [...(byFile.get(file) ?? []), cls]);
    }
    const imports = [...byFile.entries()]
        .sort(([a], [b]) => a.localeCompare(b))
        .map(([file, classes]) => `import { ${classes.join(", ")} } from "./${file}.js";`);

    const out = [
        "// Generated from createLibraryV2() / createLibraryV3() in",
        "// src/texturelab/libraries/library.cpp by scripts/port-node.ts. Don't edit;",
        "// change library.cpp and re-run `yarn port-nodes`.",
        "",
        `import { Library } from "./library.js";`,
        ...imports,
        "",
        "export function createLibraryV2(): Library {",
        "    const lib = new Library();",
        ...v2,
        "    return lib;",
        "}",
        "",
        "export function createLibraryV3(): Library {",
        "    const lib = createLibraryV2();",
        ...v3,
        "    return lib;",
        "}",
        "",
    ].join("\n");
    writeFileSync(join(tsRoot, "registry.ts"), out);
    console.log(`wrote ${relative(repoRoot, join(tsRoot, "registry.ts"))}`);
}

// ---------------------------------------------------------------------------

const classFiles = new Map<string, string>();
for (const version of ["v1", "v2", "v3"]) {
    const dir = join(cppRoot, version);
    for (const f of readdirSync(dir).filter((f) => f.endsWith(".cpp")).sort()) {
        const rel = `${version}/${basename(f, ".cpp")}`;
        if (only.length && !only.includes(rel)) {
            // Still need class names for the registry.
            const text = stripComments(extractRawStrings(readFileSync(join(dir, f), "utf8")).text);
            for (const m of text.matchAll(/void\s+(\w+)::init\s*\(\s*\)\s*\{/g)) classFiles.set(m[1]!, rel);
            continue;
        }
        for (const c of portFile(version, join(dir, f))) classFiles.set(c.className, c.file);
    }
}
generateRegistries(classFiles);

if (warnings.length) {
    console.warn(`\n${warnings.length} statement(s) need a manual port:`);
    for (const w of warnings) console.warn("  " + w);
    process.exitCode = 1;
}
