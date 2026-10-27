// Ported from src/texturelab/libraries/v3/makeittile.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { TextureNode } from "../../node.js";

export class MakeItTileNode extends TextureNode {
    override init(): void {
        this.title = "Make It Tile";
        this.addInput("image");
        this.addFloatProp("blend_width", "Blend Width", 0.2, 0.02, 0.5, 0.01);
        this.addBoolProp ("horizontal", "Horizontal", true);
        this.addBoolProp ("vertical", "Vertical", true);
        const source = /* glsl */ `
        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.0, 0.0, 0.0, 1.0);

            // Sample the original
            vec4 orig = texture(image, uv);

            // Build the half-period shift on enabled axes only.
            // Disabled axes use 0 shift, which means orig == shifted along
            // that axis — the blend collapses to a no-op for that direction.
            vec2 shift  = vec2(prop_horizontal ? 0.5 : 0.0,
                               prop_vertical   ? 0.5 : 0.0);
            vec2 sUV    = fract(uv + shift);
            vec4 shifted = texture(image, sUV);

            // Distance to the nearest edge on each enabled axis.
            // For disabled axes use 1.0 so the axis never reduces minD.
            float dx = prop_horizontal ? min(uv.x, 1.0 - uv.x) : 1.0;
            float dy = prop_vertical   ? min(uv.y, 1.0 - uv.y) : 1.0;
            float minD = min(dx, dy);

            // Blend weight: 0 at the seam → use shifted; 1 in centre → use original
            float bw = max(prop_blend_width, 0.001);
            float w  = smoothstep(0.0, bw, minD);

            return mix(shifted, orig, w);
        }
    `;
        this.setShaderSource(source);
    }
}
