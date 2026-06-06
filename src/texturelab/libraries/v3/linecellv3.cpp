#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// Voronoi edge (line-cell) node with independent X/Y scale.
// Renders the borders between Voronoi cells as bright lines.
// Seamless: integer-rounded grid sizes with mod()-wrapped cell hashes.
// Reference: https://iquilezles.org/articles/voronoilines/
void LineCellV3Node::init()
{
    this->title = "Line Cell";

    this->addFloatProp("scale",      "Scale",          30.0,  1.0, 256.0, 1.0);
    this->addFloatProp("scaleX",     "Scale X",         1.0,  0.1,  10.0, 0.1);
    this->addFloatProp("scaleY",     "Scale Y",         1.0,  0.1,  10.0, 0.1);
    this->addBoolProp ("invert",     "Invert",         false);
    this->addFloatProp("entropy",    "Order",          0.0,  0.0,  1.0, 0.01);
    this->addFloatProp("intensity",  "Intensity",      1.0,  0.0,  2.0, 0.01);
    this->addFloatProp("thickness",  "Line Thickness", 0.05, 0.0,  0.3, 0.005);

    this->setShaderSource(R""""(
        // Returns (edgeDist, nearestDiff.xy).
        // edgeDist = perpendicular distance to the nearest Voronoi edge.
        vec3 voronoiEdge(vec2 scaledUV, float gsX, float gsY)
        {
            vec2 i_st = floor(scaledUV);
            vec2 f_st = fract(scaledUV);
            vec2 seedOff = vec2(_seed * 0.173, _seed * 0.251);

            // Pass 1: find nearest cell.
            float md = 1e9;
            vec2 mg  = vec2(0.0);
            vec2 mr  = vec2(0.0);

            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    vec2 neighbor = vec2(float(x), float(y));
                    vec2 wrapped  = mod(i_st + neighbor, vec2(gsX, gsY));
                    vec2 point    = hash22(wrapped + seedOff);
                    point = mix(point, vec2(0.5), prop_entropy);
                    vec2 diff = neighbor + point - f_st;
                    float dist = length(diff);
                    if (dist < md) {
                        md = dist;
                        mr = diff;
                        mg = neighbor;
                    }
                }
            }

            // Pass 2: distance to bisector of nearest and second-nearest cells.
            float edgeDist = 1e9;
            for (int j = -2; j <= 2; j++) {
                for (int i = -2; i <= 2; i++) {
                    vec2 neighbor = mg + vec2(float(i), float(j));
                    vec2 wrapped  = mod(i_st + neighbor, vec2(gsX, gsY));
                    vec2 point    = hash22(wrapped + seedOff);
                    point = mix(point, vec2(0.5), prop_entropy);
                    vec2 diff = neighbor + point - f_st;
                    if (dot(mr - diff, mr - diff) > 0.00001)
                        edgeDist = min(edgeDist,
                            dot(0.5 * (mr + diff), normalize(diff - mr)));
                }
            }

            return vec3(edgeDist, mr);
        }

        vec4 process(vec2 uv)
        {
            float gsX = max(1.0, floor(prop_scale * prop_scaleX + 0.5));
            float gsY = max(1.0, floor(prop_scale * prop_scaleY + 0.5));

            vec2 scaledUV = vec2(uv.x * gsX, uv.y * gsY);
            vec3 c = voronoiEdge(scaledUV, gsX, gsY);

            // c.x = 0 at edge centre, grows away from it.
            // Smooth bright line that fades to black past `thickness`.
            float edge = 1.0 - smoothstep(0.0, prop_thickness, c.x);
            vec3 color = vec3(edge);

            if (prop_invert) color = 1.0 - color;
            return vec4(color * prop_intensity, 1.0);
        }
    )"""");
}
