import { DesignerNode } from "../../nodetest";

export class BlendNode extends DesignerNode
{
    public init()
    {
        this.title = "Blend";

        this.addInput("colorA");
        this.addInput("colorB");

        this.addEnumProperty("type","Type",["Multiply","Add","Subtract"]);

        var source = `
        vec4 sample(vec2 uv)
        {
            vec4 colA = texture2D(colorA,uv);
            vec4 colB = texture2D(colorB,uv);
            vec4 col = vec4(1.0);
            if (prop_type==0)
                col.rgb = colA.rgb * colB.rgb;
            if (prop_type==1)
                col.rgb = colA.rgb + colB.rgb;
            if (prop_type==2)
                col.rgb = colA.rgb - colB.rgb;

            return col;
        }
        `;

        this.buildShader(source);
    }
}