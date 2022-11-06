#pragma once

#include "../models.h"

class PolygonNode : public TextureNode {
public:
    virtual void init() override
    {
        this->title = "Polygon";

        this->addFloatProp("radius", "Radius", 0.2, 0, 1, 0.01);

        // todo: add props
        this->setShaderSource(R""""(
        vec4 process(vec2 uv)
        {
            //float prop_radius = 0.35;

            float dist = distance(uv, vec2(0.5));
            if( dist <= prop_radius) {
                return vec4(vec3(1.0), 1.0);
                
            }

            return vec4(vec3(0.0), 1.0);
        }
        )"""");
    }
};

class ColorNode : public TextureNode {
public:
    virtual void init() override
    {
        this->title = "Color";

        this->addInput("image");

        // todo: add props
        this->setShaderSource(R""""(
        vec4 process(vec2 uv)
        {
            vec4 prop_color = vec4(1,1,1,1);
            return texture(image,uv) * prop_color;
            //return vec4(uv, vec2(0.0,1.0));// * prop_color;
        }
        )"""");
    }
};

class BlendNode : public TextureNode {
public:
    virtual void init() override
    {
        this->title = "Blend";

        this->addInput("colorA");
        this->addInput("colorB");
        this->addInput("opacity");

        // todo: add props
        this->setShaderSource(R""""(
        vec4 process(vec2 uv)
        {
            return vec4(1,1,1,1);
        }
        )"""");
    }
};

class OutputNode : public TextureNode {
public:
    virtual void init() override
    {
        this->title = "Output";

        this->addInput("image");

        // todo: add props
        this->setShaderSource(R""""(
        vec4 process(vec2 uv)
        {
            vec4 prop_color = vec4(1,1,1,1);
            return texture(image,uv) * prop_color;
        }
        )"""");
    }
};