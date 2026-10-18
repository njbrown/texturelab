// Parsing and loading of .texture files. Mirrors Project::loadTextureFromJson
// (src/texturelab/project.cpp) and LibraryVersionMigrator's version detection.
//
// Loading never migrates: a file renders with the library it was saved with,
// like the desktop app does when the user declines the upgrade.

import { getInt, getLong, isObject } from "./json.js";
import type { Library } from "./libraries/library.js";
import { createLibraryV2, createLibraryV3 } from "./libraries/registry.js";
import type { TextureNode } from "./node.js";

export type LibraryVersion = "v1" | "v2" | "v3";

export const TEXTURE_CHANNELS = ["albedo", "normal", "metalness", "roughness", "height", "alpha", "ao"] as const;
export type TextureChannel = (typeof TEXTURE_CHANNELS)[number];

export class TextureParseError extends Error {
    override name = "TextureParseError";
}

export interface TextureConnection {
    leftNodeId: string;
    rightNodeId: string;
    rightNodeInput: string;
}

/** A loaded .texture graph, ready to evaluate. */
export interface TextureDocument {
    libraryVersion: LibraryVersion;
    /** Resolution the file was saved with (power of two, 32..4096). */
    width: number;
    height: number;
    randomSeed: number;
    nodes: Map<string, TextureNode>;
    connections: TextureConnection[];
    /** Channel -> node id, from editor.textureChannels. */
    textureChannels: Partial<Record<TextureChannel, string>>;
    exportFilePattern: string;
    /** typeNames the library couldn't create; those nodes were skipped. */
    skippedTypes: string[];
}

export interface ParseOptions {
    /** Reject files with more nodes than this. Default 2000. */
    maxNodes?: number;
}

// v2 typeNames that were removed or renamed in v3 (oldTypeName != newTypeName
// in LibraryVersionMigrator's v2ToV3 table). Their presence in a file without
// a libraryVersion marks it as legacy.
const LEGACY_TYPE_NAMES = new Set([
    "floodfill",
    "floodfillsampler",
    "floodfilltobbox",
    "floodfilltocolor",
    "floodfilltogradient",
    "floodfilltorandomcolor",
    "floodfilltorandomintensity",
    "bevel",
    "perlin3d",
]);

function detectLibraryVersion(json: Record<string, unknown>, nodes: unknown[]): LibraryVersion {
    const str = typeof json.libraryVersion === "string" ? json.libraryVersion.trim().toLowerCase() : "";
    if (str === "v1" || str === "v2" || str === "v3") return str;
    if (str !== "") return "v3"; // unknown string -> current, like libVersionFromString

    const legacy = nodes.some((n) => isObject(n) && LEGACY_TYPE_NAMES.has(String(n.typeName)));
    return legacy ? "v2" : "v3";
}

const libraries = new Map<LibraryVersion, Library>();

/** v1 and v2 both use createLibraryV2(), as in createLibraryForVersion(). */
export function libraryForVersion(version: LibraryVersion): Library {
    let lib = libraries.get(version);
    if (!lib) {
        lib = version === "v3" ? createLibraryV3() : createLibraryV2();
        libraries.set(version, lib);
    }
    return lib;
}

/** Node typeNames this runtime can load for a library version. */
export function supportedNodes(version: LibraryVersion = "v3"): string[] {
    return [...libraryForVersion(version).items.keys()].sort();
}

/** Parse a .texture file (its JSON text, or the already-parsed object). */
export function parseTexture(input: string | object, options: ParseOptions = {}): TextureDocument {
    let json: unknown = input;
    if (typeof input === "string") {
        try {
            json = JSON.parse(input);
        } catch (e) {
            throw new TextureParseError(`Not valid JSON: ${(e as Error).message}`);
        }
    }
    if (!isObject(json)) throw new TextureParseError("Not a .texture file: expected a JSON object");
    if (!Array.isArray(json.nodes)) throw new TextureParseError("Not a .texture file: missing `nodes` array");

    const maxNodes = options.maxNodes ?? 2000;
    if (json.nodes.length > maxNodes) {
        throw new TextureParseError(`Too many nodes (${json.nodes.length}, limit ${maxNodes})`);
    }

    const libraryVersion = detectLibraryVersion(json, json.nodes);
    const lib = libraryForVersion(libraryVersion);

    const nodes = new Map<string, TextureNode>();
    const skippedTypes: string[] = [];

    for (const item of json.nodes) {
        if (!isObject(item)) continue;
        const typeName = typeof item.typeName === "string" ? item.typeName : "";
        const node = lib.createNode(typeName);
        // Unknown/legacy typeName the resolved library can't build: skip it.
        if (!node) {
            skippedTypes.push(typeName);
            continue;
        }
        node.exportName = typeof item.exportName === "string" ? item.exportName : "";
        node.id = typeof item.id === "string" ? item.id : "";
        node.randomSeed = getLong(item.randomSeed);

        const props = isObject(item.properties) ? item.properties : {};
        for (const [key, value] of Object.entries(props)) {
            node.getProp(key)?.fromJsonValue(value);
        }

        nodes.set(node.id, node);
    }

    // Connections. Drop ones whose endpoints didn't load, and ones that would
    // close a cycle (older builds allowed cycles, which never finish rendering).
    const connections: TextureConnection[] = [];
    const upstream = new Map<string, string[]>(); // rightNodeId -> leftNodeIds
    const feedsInto = (from: string, to: string): boolean => {
        // does `from` (transitively) feed into `to`? i.e. is `from` upstream of `to`
        const stack = [to];
        const seen = new Set<string>();
        while (stack.length) {
            const id = stack.pop()!;
            if (id === from) return true;
            if (seen.has(id)) continue;
            seen.add(id);
            stack.push(...(upstream.get(id) ?? []));
        }
        return false;
    };

    for (const item of Array.isArray(json.connections) ? json.connections : []) {
        if (!isObject(item)) continue;
        const leftNodeId = String(item.leftNodeId ?? "");
        const rightNodeId = String(item.rightNodeId ?? "");
        const rightNodeInput = String(item.rightNodeInput ?? "");
        if (!nodes.has(leftNodeId) || !nodes.has(rightNodeId)) continue;
        if (feedsInto(rightNodeId, leftNodeId)) continue;

        // An input should hold one connection. Files with duplicates render in
        // an arbitrary order on desktop; keep the last one here.
        const existing = connections.findIndex(
            (c) => c.rightNodeId === rightNodeId && c.rightNodeInput === rightNodeInput,
        );
        if (existing >= 0) connections.splice(existing, 1);

        connections.push({ leftNodeId, rightNodeId, rightNodeInput });
        upstream.set(rightNodeId, [...(upstream.get(rightNodeId) ?? []), leftNodeId]);
    }

    // Project-wide render settings. Only accept resolutions the desktop
    // toolbar offers (powers of two, 32..4096).
    let width = 1024;
    const res = getInt(json.textureWidth, width);
    if (res >= 32 && res <= 4096 && (res & (res - 1)) === 0) width = res;

    const textureChannels: Partial<Record<TextureChannel, string>> = {};
    const editor = isObject(json.editor) ? json.editor : {};
    const channels = isObject(editor.textureChannels) ? editor.textureChannels : {};
    for (const channel of TEXTURE_CHANNELS) {
        const id = channels[channel];
        if (typeof id === "string" && id) textureChannels[channel] = id;
    }

    const exportObj = isObject(json.export) ? json.export : {};

    return {
        libraryVersion,
        width,
        height: width,
        randomSeed: getInt(json.randomSeed, 0),
        nodes,
        connections,
        textureChannels,
        exportFilePattern:
            typeof exportObj.filePattern === "string" ? exportObj.filePattern : "${project}_${name}",
        skippedTypes,
    };
}
