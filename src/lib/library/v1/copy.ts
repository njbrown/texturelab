import { DesignerNode } from "../../designer/designernode";

export class CopyNode extends DesignerNode {
  public init() {
    this.title = "Copy";

    this.addInput("image");
    this.addStringProperty("name", "Name");

    var source = `
        vec4 sample(vec2 uv)
        {
            vec4 col = texture2D(image, uv);
            return col;
        }
        `;

    this.buildShader(source);
  }
}
