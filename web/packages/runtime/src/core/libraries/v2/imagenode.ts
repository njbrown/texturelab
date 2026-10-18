// Ported from src/texturelab/libraries/v2/imagenode.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class ImageNode extends TextureNode {
    override init(): void {
        this.title = "Image";
        this.addImageProp("image", "Image");
        this.setShaderSource(/* glsl */ `
        vec4 process(vec2 uv)
        {
            return texture(prop_image,uv);
        }
        `);
    }
}
