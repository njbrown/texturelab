// Ported from src/texturelab/libraries/v1/invert.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class InvertNode extends TextureNode {
    override init(): void {
        this.title = "Invert";
        this.addInput("color");
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 col = vec4(1.0) - texture(color,uv);
            col.a = 1.0;
            return col;
        }
        `;
        this.setShaderSource(source);
    }
}
