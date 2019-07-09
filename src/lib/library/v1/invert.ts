import { DesignerNode } from "../../nodetest";

export class InvertNode extends DesignerNode
{
    public init()
    {
        this.title = "Invert";

        this.addInput("color");

        var source = `
        vec4 sample(vec2 uv)
        {
            vec4 col = vec4(1.0) - texture2D(color,uv);
            col.a = 1.0;
            return col;
        }
        `;

        this.buildShader(source);
    }
}