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

class GradientNode : public TextureNode {
public:
    virtual void init() override;
};

class GradientMapNode : public TextureNode {
public:
    virtual void init() override;
};

class HeightShiftNode : public TextureNode {
public:
    virtual void init() override;
};

class HexagonNode : public TextureNode {
public:
    virtual void init() override;
};

class InvertNode : public TextureNode {
public:
    virtual void init() override;
};

class LineCellNode : public TextureNode {
public:
    virtual void init() override;
};

class MapRangeNode : public TextureNode {
public:
    virtual void init() override;
};

class MaskNode : public TextureNode {
public:
    virtual void init() override;
};

class MirrorNode : public TextureNode {
public:
    virtual void init() override;
};

class NormalMapNode : public TextureNode {
public:
    virtual void init() override;
};

class OutputNode : public TextureNode {
public:
    virtual void init() override;
};

class Perlin3DNode : public TextureNode {
public:
    virtual void init() override;
};

class ShapesNode : public TextureNode {
public:
    virtual void init() override;
};

class SimplexNoiseNode : public TextureNode {
public:
    virtual void init() override;
};

class SolidCellNode : public TextureNode {
public:
    virtual void init() override;
};

class SplatNode : public TextureNode {
public:
    virtual void init() override;
};

class ThresholdNode : public TextureNode {
public:
    virtual void init() override;
};

class TileNode : public TextureNode {
public:
    virtual void init() override;
};

class Transform2DNode : public TextureNode {
public:
    virtual void init() override;
};

class WarpNode : public TextureNode {
public:
    virtual void init() override;
};

class WaveNode : public TextureNode {
public:
    virtual void init() override;
};