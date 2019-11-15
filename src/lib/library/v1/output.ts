import { DesignerNode } from "../../designer/designernode";
import { Color } from "@/lib/designer/color";

export class OutputNode extends DesignerNode {
  public init() {
    this.title = "Output";

    this.addInput("image");
    this.addColorProperty("color", "Default Color", new Color());

    var source = `
        vec4 process(vec2 uv)
        {
            vec4 col;
            if (image_connected == 1) {
              col = vec4(0,1,0,1);
              col = texture(image, uv);
              return texture(image, uv);
            } else {
              col = prop_color;
              return prop_color;
            }

            return col;
        }
        `;

    this.buildShader(source);
  }
}
