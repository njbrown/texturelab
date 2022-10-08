#pragma once

#include "../models.h"

class PolygonNode : public TextureNode
{
public:
    virtual void init() override
    {
        this->title = "Polygon";

        // todo: add props
    }
};

class ColorNode : public TextureNode
{
public:
    virtual void init() override
    {
        this->title = "Color";

        this->addInput("image");

        // todo: add props
    }
};

class BlendNode : public TextureNode
{
public:
    virtual void init() override
    {
        this->title = "Blend";

        this->addInput("colorA");
        this->addInput("colorB");
        this->addInput("opacity");

        // todo: add props
    }
};

class OutputNode : public TextureNode
{
public:
    virtual void init() override
    {
        this->title = "Blend";

        this->addInput("image");

        // todo: add props
    }
};