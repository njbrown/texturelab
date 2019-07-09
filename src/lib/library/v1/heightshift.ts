import { DesignerNode} from "../../nodetest";

export class HeightShiftNode extends DesignerNode
{
    public init()
    {
        this.title = "Height Shift";

        this.addInput("image");

        this.addFloatProperty("shift","Shift", 0.0, -1.0, 1.0, 0.01);

        var source = `
        vec4 sample(vec2 uv)
        {
            vec4 a =  texture2D(image, uv);

            return a + vec4(vec3(prop_shift), 0.0);
        }
        `;

        this.buildShader(source);
    }
}