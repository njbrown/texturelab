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
