// Ported from src/texturelab/libraries/v3/fastao.cpp by scripts/port-node.ts.
// Keep the GLSL and props in sync with the C++ when either changes.

import { MultiPassNode } from "../../node.js";

// TODO(port): the C++ node renders with a custom multi-pass
// NodeTextureRenderer, which isn't ported yet. init() is ported so files
// containing this node load and report it as unsupported.

export class FastAONode extends MultiPassNode {
    override init(): void {
        this.title = "Fast AO";
        this.addInput("height");
        this.addFloatProp("radius", "Radius", 0.01, 0.001, 1.0, 0.005);
        this.addIntProp("samples", "Samples", 48, 8, 256, 8);
        const qualityEnum = this.addEnumProp("quality", "Quality", ["Draft", "Medium", "High", "Ultra"]);
        qualityEnum.index = 1;
        this.addFloatProp("intensity", "Intensity", 1.0, 0.0, 5.0, 0.1);
        this.addFloatProp("bias", "Bias", 0.02, 0.0, 0.5, 0.005);
        this.addFloatProp("height_scale", "Height Scale", 1.0, 0.01, 8.0, 0.01);
        this.addFloatProp("detail", "Detail", 0.35, 0.0, 1.0, 0.01);
        this.addFloatProp("detail_scale", "Detail Scale", 0.15, 0.02, 0.5, 0.01);
        this.addFloatProp("denoise", "Denoise", 0.5, 0.0, 1.0, 0.01);
        this.addFloatProp("contrast", "Contrast", 0.5, 0.25, 4.0, 0.05);
        this.addBoolProp("cone", "Cone Filtering", true);
    }
}
