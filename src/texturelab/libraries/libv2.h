#pragma once

#include "../models.h"

class CircleNode : public TextureNode {
public:
    virtual void init() override
    {
        this->title = "Circle";

        this->addFloatProp("radius", "Radius", 0.2, 0, 1, 0.01);
        this->addEnumProp("color_gen", "Color Generation",
                          {"Flat", "Linear", "Exponential"});

        this->setShaderSource(R""""(
        vec4 process(vec2 uv)
        {
            float dist = distance(uv, vec2(0.5));
            if( dist <= prop_radius) {
                if (prop_color_gen==0)
                    return vec4(vec3(1.0), 1.0);
                else if (prop_color_gen==1)
                    return vec4(vec3(1.0 - dist / prop_radius), 1.0);
                else if (prop_color_gen==2)
                {
                    float val = dist / prop_radius;
                    return vec4(vec3(1.0 - val * val), 1.0);
                }
                
            }

            return vec4(vec3(0.0), 1.0);
        }
        )"""");
    }
};

class PolygonNode : public TextureNode {
public:
    virtual void init() override
    {
        this->title = "Polygon";

        this->addFloatProp("radius", "Radius", 0.2, 0, 1, 0.01);
        this->addFloatProp("angle", "Angle", 0, 0, 360, 0.1);
        this->addIntProp("sides", "Sides", 5, 0, 20, 1);
        this->addFloatProp("gradient", "Gradient", 0, 0, 1.0, 0.01);

        // todo: add props
        this->setShaderSource(R""""(
        #define PI 3.14159265359
        #define TWO_PI 6.28318530718

        float linearstep(float a, float b, float t)
        {
            if (t <= a) return 0.0;
            if (t >= b) return 1.0;

            return (t-a)/(b-a);
        }

        vec4 process(vec2 uv)
        {
            uv = uv *2.-1.;

            // Angle and radius from the current pixel
            float a = atan(uv.x,uv.y)+radians(prop_angle);
            float r = TWO_PI/float(prop_sides);

            float d = cos(floor(.5+a/r)*r-a)*length(uv) / prop_radius;

            vec3 color = vec3(1.0-linearstep(0.8-prop_gradient,0.8,d));

            return vec4(color, 1.0);
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

        this->addEnumProp("type", "Type",
                          {"Multiply", "Add", "Subtract", "Divide",
                           //   "Add Sub",
                           "Max", "Min", "Switch", "Overlay", "Screen"});
        this->addFloatProp("opacity", "Opacity", 1.0, 0.0, 1.0, 0.01);

        // todo: add props
        this->setShaderSource(R""""(
        float screen(float fg, float bg) {
            float res = (1.0 - fg) * (1.0 - bg);
            return 1.0 - res;
        }
        vec4 process(vec2 uv)
        {
            float finalOpacity = prop_opacity;
            if (opacity_connected)
                finalOpacity *= texture(opacity, uv).r;

            vec4 colA = texture(colorA,uv);
            vec4 colB = texture(colorB,uv);
            vec4 col = vec4(1.0);

            if (prop_type==0){ // multiply
                col.rgb = colA.rgb * colB.rgb;
            }
            if (prop_type==1) // add
                col.rgb = colA.rgb + colB.rgb;
            if (prop_type==2) // subtract
                col.rgb = colB.rgb - colA.rgb;
            if (prop_type==3) // divide
                col.rgb = colB.rgb / colA.rgb;
            // if (prop_type==4) {// add sub
            //     if (colA.r > 0.5) col.r = colB.r + colA.r; else col.r = colB.r - colA.r;
            //     if (colA.g > 0.5) col.g = colB.g + colA.g; else col.g = colB.g - colA.g;
            //     if (colA.b > 0.5) col.b = colB.b + colA.b; else col.b = colB.b - colA.b;
            // }
            if (prop_type==4) { // max
                col.rgb = max(colA.rgb, colB.rgb);
            }
            if (prop_type==5) { // min
                col.rgb = min(colA.rgb, colB.rgb);
            }
            if (prop_type==6) { // switch
                col.rgb = colA.rgb;
            }
            if (prop_type==7) { // overlay
                if (colB.r < 0.5) col.r = colB.r * colA.r; else col.r = screen(colB.r, colA.r);
                if (colB.g < 0.5) col.g = colB.g * colA.g; else col.g = screen(colB.g, colA.g);
                if (colB.b < 0.5) col.b = colB.b * colA.b; else col.b = screen(colB.b, colA.b);
            }
            if (prop_type==8) { // screen
                col.r = screen(colA.r, colB.r);
                col.g = screen(colA.g, colB.g);
                col.b = screen(colA.b, colB.b);
            }

            // apply opacity
            col.rgb = mix(colB.rgb, col.rgb, vec3(finalOpacity));

            return col;
        }
        )"""");
    }
};

class NormalMapNode : public TextureNode {
public:
    virtual void init() override
    {
        this->title = "Normal Map";

        this->addInput("height");

        this->addFloatProp("strength", "Strength", 1, -2.0, 2.0, 0.05);
        this->addIntProp("range", "Range", 1, 1, 20, 1);
        this->addBoolProp("res_ind", "Resolution Independent", false);

        this->setShaderSource(R""""(
        vec4 process(vec2 uv)
        {
            vec2 step = (vec2(1.0, 1.0) / _textureSize) * float(prop_range);
            if (prop_res_ind)
                step = (vec2(1.0, 1.0) / 1024.0) * float(prop_range);
            float strength = prop_strength * 0.1;

            // center point
            float d0 = abs(texture(height, uv + vec2(0.0, 0.0)).r) * strength / 2.0;

            // sample horizontally
            float d1 = abs(texture(height, uv + vec2(step.x, 0.0)).r) * strength / 2.0;
            float d2 = abs(texture(height, uv + vec2(-step.x, 0.0)).r) * strength / 2.0;

            // sample vertically
            float d3 = abs(texture(height, uv + vec2(0.0, step.y)).r) * strength / 2.0;
            float d4 = abs(texture(height, uv + vec2(0.0, -step.y)).r) * strength / 2.0;

            // find diff horizontally and average
            float dx = ((d2 - d0) + (d0 - d1)) * 0.5;

            // find diff vertically and average
            float dy = ((d4 - d0) + (d0 - d3)) * 0.5;

            vec3 dvx = vec3(step.x, 0.0       , d1-d0);
            vec3 dvy = vec3(0.0       , step.y, d3-d0);
            vec3 normal = normalize(cross(dvx, dvy));
            vec3 final = normal.xyz * 0.5 + 0.5;

            return vec4(final, 1.0);
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