// Ported from src/texturelab/libraries/v1/output.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";
import { QColor } from "../../color.js";

export class OutputNode extends TextureNode {
    override init(): void {
        this.title = "Output";
        this.addInput("image");
        this.addStringProp("name", "Output Name");
        this.addEnumProp("components", "Components", [ "RGBA", "RGB", "R", "G", "B", "A", ]);
        this.addEnumProp("precision", "Precision", [ "8 Bits Per Component", "16 Bits Per Component", ]);
        this.addColorProp("color", "Default Color", QColor(0, 0, 0, 255));
        this.setShaderSource(/* glsl */ `
        vec4 process(vec2 uv)
        {
            vec4 col;
            if (image_connected) {
              col = vec4(0,1,0,1);
              col = texture(image, uv);
            //   return texture(image, uv);
            } else {
              col = prop_color;
            //   return prop_color;
            }

			if(prop_components == 2) col.rgb = vec3(col.r);
			if(prop_components == 3) col.rgb = vec3(col.g);
			if(prop_components == 4) col.rgb = vec3(col.b);
			if(prop_components == 5) col.rgb = vec3(col.a);

            return col;
        }
        `);
    }
}
