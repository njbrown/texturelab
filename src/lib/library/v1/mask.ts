import { DesignerNode } from "../../designer/designernode";

export class MaskNode extends DesignerNode {
  public init() {
    this.title = "Mask";

    this.addInput("textureA");
    this.addInput("textureB");
    this.addInput("mask");

    var source = `
        float lum(vec4 col)
        {
            return (col.r + col.g + col.b) / 3.0;
        }

        vec4 sample(vec2 uv)
        {
            vec4 a =  texture2D(textureA, uv);
            vec4 b =  texture2D(textureB, uv);
            vec4 m =  texture2D(mask, uv);
            float t = lum(m);

            // lerp
            return a * t + b * (1.0 - t);
        }
        `;

    this.buildShader(source);
  }
}
