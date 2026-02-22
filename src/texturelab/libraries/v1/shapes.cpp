#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"

void CircleNode::init()
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

void PolygonNode::init()
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