#pragma once

#include "../models.h"

class AdvanceSplatterNode : public TextureNode {
public:
    virtual void init() override;
};

class AnisotropicBlurNode : public TextureNode {
public:
    virtual void init() override;
};

class BevelNode : public TextureNode {
public:
    virtual void init() override;
};

class BlurNodeV2 : public TextureNode {
public:
    virtual void init() override;
};

class CapsuleNode : public TextureNode {
public:
    virtual void init() override;
};

class CartesianToPolarNode : public TextureNode {
public:
    virtual void init() override;
};

class CircularSplatterNode : public TextureNode {
public:
    virtual void init() override;
};

class ClampNode : public TextureNode {
public:
    virtual void init() override;
};

class CombineNormalsNode : public TextureNode {
public:
    virtual void init() override;
};

class DirectionalBlurNode : public TextureNode {
public:
    virtual void init() override;
};

class DirectionalWarpV2Node : public TextureNode {
public:
    virtual void init() override;
};

class ExtractChannelNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillSamplerNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillToBBoxNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillToColorNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillToGradientNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillToRandomColorNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillToRandomIntensityNode : public TextureNode {
public:
    virtual void init() override;
};

class GradientDynamicNode : public TextureNode {
public:
    virtual void init() override;
};

class GradientNoiseNode : public TextureNode {
public:
    virtual void init() override;
};

class GradientNoiseFractalSumNode : public TextureNode {
public:
    virtual void init() override;
};

class GrayscaleNode : public TextureNode {
public:
    virtual void init() override;
};

class HistogramScanNode : public TextureNode {
public:
    virtual void init() override;
};

class HistogramSelectNode : public TextureNode {
public:
    virtual void init() override;
};

class HistogramShiftNode : public TextureNode {
public:
    virtual void init() override;
};
class HslNode : public TextureNode {
public:
    virtual void init() override;
};

class HslExtractNode : public TextureNode {
public:
    virtual void init() override;
};

class ImageNode : public TextureNode {
    ImageProp* imageProp;

public:
    virtual void init() override;
};

class InvertNormalNode : public TextureNode {
public:
    virtual void init() override;
};

class NormalMapV2Node : public TextureNode {
public:
    virtual void init() override;
};

class PolarToCartesianNode : public TextureNode {
public:
    virtual void init() override;
};

class PowNode : public TextureNode {
public:
    virtual void init() override;
};

class QuantizeNode : public TextureNode {
public:
    virtual void init() override;
};

class RgbaMergeNode : public TextureNode {
public:
    virtual void init() override;
};

class RgbaShuffleNode : public TextureNode {
public:
    virtual void init() override;
};

class SimplexNoiseNodeV2 : public TextureNode {
public:
    virtual void init() override;
};

class SkewNode : public TextureNode {
public:
    virtual void init() override;
};

class SlopeBlurNode : public TextureNode {
public:
    virtual void init() override;
};

class SoftFlowerNode : public TextureNode {
public:
    virtual void init() override;
};

class SplatV2Node : public TextureNode {
public:
    virtual void init() override;
};

class StarNode : public TextureNode {
public:
    virtual void init() override;
};

class StripesNode : public TextureNode {
public:
    virtual void init() override;
};

class TileSamplerNode : public TextureNode {
public:
    virtual void init() override;
};

class Transform2DV2Node : public TextureNode {
public:
    virtual void init() override;
};

class ValueNoiseNode : public TextureNode {
public:
    virtual void init() override;
};

class ValueNoiseFractalSumNode : public TextureNode {
public:
    virtual void init() override;
};

class WarpNodeV2 : public TextureNode {
public:
    virtual void init() override;
};