#pragma once

#include "../models.h"
#include "v3/curvenode.h"
#include <memory>

// ---------------------------------------------------------------------------
// Phase 1 — Single-pass filter / color nodes
// ---------------------------------------------------------------------------

class EdgeDetectNode : public TextureNode {
public:
    void init() override;
};

class HighpassNode : public TextureNode {
public:
    void init() override;
};

class EmbossNode : public TextureNode {
public:
    void init() override;
};

class VibranceNode : public TextureNode {
public:
    void init() override;
};

class ColorToMaskNode : public TextureNode {
public:
    void init() override;
};

class ToonGradientNode : public TextureNode {
public:
    void init() override;
};

class AutoLevelsNode : public TextureNode {
public:
    void init() override;
    std::shared_ptr<NodeTextureRenderer> createRenderer() override;
    std::shared_ptr<NodeRenderData>      createRenderData() override;
};

// ---------------------------------------------------------------------------
// Phase 2 — Generator nodes
// ---------------------------------------------------------------------------

class DirectionalScratchesNode : public TextureNode {
public:
    void init() override;
};

class RoughGrainNode : public TextureNode {
public:
    void init() override;
};

class VoronoiFractalNode : public TextureNode {
public:
    void init() override;
};

class TruchetNode : public TextureNode {
public:
    void init() override;
};

class FBMDomainWarpNode : public TextureNode {
public:
    void init() override;
};

// ---------------------------------------------------------------------------
// Phase 3 — Multi-pass nodes
// ---------------------------------------------------------------------------

class BlurHQNode : public TextureNode {
public:
    void init() override;
    std::shared_ptr<NodeTextureRenderer> createRenderer() override;
    std::shared_ptr<NodeRenderData>      createRenderData() override;
};

class DistanceTransformNode : public TextureNode {
public:
    void init() override;
    std::shared_ptr<NodeTextureRenderer> createRenderer() override;
    std::shared_ptr<NodeRenderData>      createRenderData() override;
};

class HeightBlendNode : public TextureNode {
public:
    void init() override;
};

class MakeItTileNode : public TextureNode {
public:
    void init() override;
};

class BevelV2Node : public TextureNode {
public:
    virtual void init() override;
    std::shared_ptr<NodeTextureRenderer> createRenderer() override;
    std::shared_ptr<NodeRenderData> createRenderData() override;
};

class AmbientOcclusionNode : public TextureNode {
public:
    virtual void init() override;
};

class CurvatureNode : public TextureNode {
public:
    virtual void init() override;
};

class MaskedBlurNode : public TextureNode {
public:
    virtual void init() override;
};

class RaysNode : public TextureNode {
public:
    virtual void init() override;
};

class SwirlNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillV2Node : public TextureNode {
public:
    virtual void init() override;
    std::shared_ptr<NodeTextureRenderer> createRenderer() override;
    std::shared_ptr<NodeRenderData> createRenderData() override;
};

class FloodFillV2ToColorNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillV2ToRandomColorNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillV2ToRandomIntensityNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillV2ToBBoxNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillV2ToGradientNode : public TextureNode {
public:
    virtual void init() override;
};

class FloodFillV2SamplerNode : public TextureNode {
public:
    virtual void init() override;
};
