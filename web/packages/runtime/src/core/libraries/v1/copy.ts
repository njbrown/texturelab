// Ported from src/texturelab/libraries/v1/copy.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class CopyNode extends TextureNode {
    override init(): void {
        this.title = "Copy";
        this.addInput("image");
        this.addStringProp("name", "Name");
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);
            return col;
        }
        `;
        this.setShaderSource(source);
    }
}
