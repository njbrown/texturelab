#pragma once

#include "../models.h"

class CircleNode : public TextureNode {
public:
    virtual void init() override;
};

class PolygonNode : public TextureNode {
public:
    virtual void init() override;
};

class ColorNode : public TextureNode {
public:
    virtual void init() override;
};

class BlendNode : public TextureNode {
public:
    virtual void init() override;
};

// class OutputNode : public TextureNode {
// public:
//     virtual void init() override
//     {
//         this->title = "Output";

//         this->addInput("image");

//         // todo: add props
//         this->setShaderSource(R""""(
//         vec4 process(vec2 uv)
//         {
//             vec4 prop_color = vec4(1,1,1,1);
//             return texture(image,uv) * prop_color;
//         }
//         )"""");
//     }
// };