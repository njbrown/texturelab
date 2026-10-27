// Mirrors Library in src/texturelab/libraries/library.h.

import type { TextureNode } from "../node.js";

export interface LibraryEntry {
    name: string;
    displayName: string;
    factory: new () => TextureNode;
}

export class Library {
    readonly items = new Map<string, LibraryEntry>();

    addNode(name: string, displayName: string, factory: new () => TextureNode): void {
        this.items.set(name, { name, displayName, factory });
    }

    removeNode(name: string): void {
        this.items.delete(name);
    }

    hasNode(name: string): boolean {
        return this.items.has(name);
    }

    createNode(name: string): TextureNode | null {
        const item = this.items.get(name);
        if (!item) return null;
        const node = new item.factory();
        node.typeName = name;
        node.init();
        return node;
    }
}
