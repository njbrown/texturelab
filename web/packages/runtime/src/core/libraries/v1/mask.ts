// Ported from src/texturelab/libraries/v1/mask.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class MaskNode extends TextureNode {
    override init(): void {
        this.title = "Mask";
        this.addInput("textureA");
        this.addInput("textureB");
        this.addInput("mask");
        const source = /* glsl */ `
       float lum(vec4 col)
        {
            return (col.r + col.g + col.b) / 3.0;
        }

        vec4 process(vec2 uv)
        {
            vec4 a =  texture(textureA, uv);
            vec4 b =  texture(textureB, uv);
            vec4 m =  texture(mask, uv);
            float t = lum(m);

            // lerp
            return a * t + b * (1.0 - t);
        }
        `;
        this.setShaderSource(source);
    }
}
