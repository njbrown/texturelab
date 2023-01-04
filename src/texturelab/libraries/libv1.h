#pragma once

#include "../models.h"

class BrickGeneratorNode : public TextureNode {
public:
    virtual void init() override;
};

class BrightnessContrastNode : public TextureNode {
public:
    virtual void init() override;
};

class CellNode : public TextureNode {
public:
    virtual void init() override;
};

class CheckerboardNode : public TextureNode {
public:
    virtual void init() override;
};

class CopyNode : public TextureNode {
public:
    virtual void init() override;
};

class DirectionalWarpNode : public TextureNode {
public:
    virtual void init() override;
};

class FractalNoiseNode : public TextureNode {
public:
    virtual void init() override;
};