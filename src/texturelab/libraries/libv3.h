#pragma once

#include "../models.h"

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
