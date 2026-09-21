#include "../../models.h"
#include "../../props.h"
#include "../libv1.h"
#include <QColor>
#include <QList>

void GradientNode::init()
{
    this->title = "Gradient";

    QColor white(255, 255, 255);

    this->addColorProp("colorA", "Color A", QColor(0, 0, 0));
    this->addFloatProp("posA", "Position A", 0, 0, 1, 0.01);
    this->addColorProp("colorB", "Color B", white);
    this->addFloatProp("posB", "Position B", 1, 0, 1, 0.01);

    this->addEnumProp(
        "mode", "Gradient Direction",
        {"Left To Right", "Right To Left", "Top To Bottom", "Bottom To Top"});

    auto source = R""""(
        #define POINTS_MAX 32

        // assumes points are sorted
        vec3 calcGradient(float t, vec3 colors[POINTS_MAX], float positions[POINTS_MAX], int numPoints)
        {
            if (numPoints == 0)
                return vec3(1,0,0);
            
            if (numPoints == 1)
                return colors[0];
            
            // here at least two points are available
            if (t < positions[0])
                return colors[0];
            
            int last = numPoints - 1;
            if (t > positions[last])
                return colors[last];
            
            // find two points in-between and lerp
            
            for(int i = 0; i < numPoints-1;i++) {
                if (positions[i+1] > t) {
                    vec3 colorA = colors[i];
                    vec3 colorB = colors[i+1];
                    
                    float t1 = positions[i];
                    float t2 = positions[i+1];
                    
                    float lerpPos = (t - t1)/(t2 - t1);
                    return mix(colorA, colorB, lerpPos);
                    
                }
                
            }
            
            return vec3(0,0,0);
        }


        vec4 process(vec2 uv)
        {
            float t = 0.0;
    
            // left to right
            if (prop_mode == 0)
                t = uv.x;
            // right to left
            else if (prop_mode == 1)
                t = 1.0 - uv.x;
            // top to bottom
            else if (prop_mode == 2)
                t = 1.0 - uv.y;
            // bottom to top
            else if (prop_mode == 3)
                t = uv.y;


            vec3 colors[POINTS_MAX];
            colors[0] = prop_colorA.rgb;
            colors[1] = prop_colorB.rgb;
            float positions[POINTS_MAX];
            positions[0] = prop_posA;
            positions[1] = prop_posB;
                
            
            vec3 col = calcGradient(t, colors, positions, 2);
            
            return vec4(col,1.0);
        }
        )"""";

    this->setShaderSource(source);
}