#pragma once

#include "../models.h"
#include "v3/curvenode.h"
#include <memory>

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
