// Ported from src/texturelab/libraries/v3/curvenode.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class CurveNode extends TextureNode {
    override init(): void {
        this.title = "Curve";
        this.addInput("image");
        this.addCurveProp("curve", "Curve");
        const source = /* glsl */ `
        vec4 process(vec2 uv) {
            vec4  col    = texture(image, uv);
            float gray   = (col.r + col.g + col.b) * 0.3333333;
            float mapped = evalCurve(prop_curve, gray);
            return vec4(vec3(mapped), col.a);
            // return vec4(1.0,0.0,0.0,1.0);
        }
    `;
        this.setShaderSource(source);
    }
}
