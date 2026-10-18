// Ported from src/texturelab/libraries/v2/grayscale.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class GrayscaleNode extends TextureNode {
    override init(): void {
        this.title = "Grayscale";
        this.addInput("image");
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            // grayscale input color
            vec4 col = texture(image, uv);
			
			col.rgb = vec3((col.r + col.g + col.b) * 0.3333333);

            return col;
        }
          `;
        this.setShaderSource(source);
    }
}
