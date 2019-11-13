import { DesignerNode } from "../../designer/designernode";

export class CopyNode extends DesignerNode {
  public init() {
    this.title = "Copy";

    this.addInput("image");
    this.addStringProperty("name", "Name");

    var source = `
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);
            return col;
        }
        `;

    this.buildShader(source);
  }
}
