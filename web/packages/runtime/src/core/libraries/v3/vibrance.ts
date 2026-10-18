// Ported from src/texturelab/libraries/v3/vibrance.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class VibranceNode extends TextureNode {
    override init(): void {
        this.title = "Vibrance";
        this.addInput("image");
        this.addFloatProp("vibrance", "Vibrance", 0.3, -1.0, 1.0, 0.05);
        this.addFloatProp("saturation", "Saturation", 0.0, -1.0, 1.0, 0.05);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.0, 0.0, 0.0, 1.0);

            vec4 col  = texture(image, uv);
            float lum = dot(col.rgb, vec3(0.2126, 0.7152, 0.0722));
            vec3 gray = vec3(lum);

            // --- Uniform saturation (applied first) ---
            vec3 saturated = mix(gray, col.rgb, 1.0 + prop_saturation);

            // --- Vibrance: pixel's current saturation range [0..1] ---
            float maxC = max(saturated.r, max(saturated.g, saturated.b));
            float minC = min(saturated.r, min(saturated.g, saturated.b));
            float sat  = maxC - minC; // 0 = fully gray, 1 = fully saturated

            // Protection factor: low-sat pixels get most boost, saturated pixels get none
            float protection = 1.0 - sat;

            // Apply vibrance modulated by protection factor
            float boostLum = dot(saturated, vec3(0.2126, 0.7152, 0.0722));
            vec3  boostGray = vec3(boostLum);
            vec3  result = mix(boostGray, saturated,
                               1.0 + prop_vibrance * protection);

            return vec4(clamp(result, 0.0, 1.0), col.a);
        }
    `;
        this.setShaderSource(source);
    }
}
