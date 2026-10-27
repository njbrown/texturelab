// Ported from src/texturelab/libraries/v3/toongradient.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";
import { Gradient } from "../../gradient.js";

export class ToonGradientNode extends TextureNode {
    override init(): void {
        this.title = "Toon Gradient";
        this.addInput("image");
        this.addIntProp ("bands", "Bands", 4, 2, 16, 1);
        this.addFloatProp("softness", "Edge Softness", 0.05, 0.0, 0.5, 0.01);
        this.addGradientProp("gradient", "Gradient", Gradient.defaultGradient());
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.0, 0.0, 0.0, 1.0);

            float value = texture(image, uv).r; // not "input": reserved in GLSL 1.50
            float bands = float(prop_bands);

            // Quantise to N bands
            float bandIdx   = floor(value * bands);
            float bandFract = fract(value * bands);

            // Soft transition between bands using smoothstep on the intra-band fraction
            // smoothstep is undefined with equal edges, i.e. softness 0
            float softness = max(prop_softness, 1e-4);
            float soft = smoothstep(0.0, softness * bands, bandFract)
                       * (1.0 - smoothstep((1.0 - softness) * bands,
                                            bands, bandFract + bandIdx * 1.0));

            // Map to [0,1] for gradient sampling
            float t = (bandIdx + clamp(soft, 0.0, 1.0)) / bands;
            t = clamp(t, 0.0, 1.0);

            vec3 col = sampleGradient(prop_gradient, t);
            return vec4(col, 1.0);
        }
    `;
        this.setShaderSource(source);
    }
}
