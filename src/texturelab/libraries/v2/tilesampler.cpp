#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

TileSamplerNode::init()
{
    this->title = "Tile Sampler";

    this->addInput("image");
    this->addInput("mask");
    this->addInput("size");
    this->addInput("intensity");
    this->addInput("vector");

    this->addEnumProp("blendType", "Blend Type", [ "Max", "Add" ]);

    this->addIntProp("rows", "Row Count", 8, 0, 15, 1);
    this->addIntProp("columns", "Column Count", 8, 0, 15, 1);

    const posProps = this.createGroup("Position");
    posProps.add(this->addFloatProp("offset", "Offset", 0.5, 0, 1, 0.1));
    posProps.add(this->addEnumProp("offset_axis", "Offset Axis",
                                   [ "X Axis", "Y Axis" ]));
    posProps.add(
        this->addIntProp("offset_interval", "Offset Interval", 1, 1, 5, 1));

    const rotProps = this.createGroup("Rotation");
    rotProps.add(this->addFloatProp("rot", "Rotation", 0, 0, 360, 0.1));
    rotProps.add(
        this->addFloatProp("rotRand", "Random Rotation", 0, 0, 1.0, 0.01));
    rotProps.add(
        this->addFloatProp("posRand", "Random Position", 0, 0, 1.0, 0.01));
    rotProps.add(this->addFloatProp("intensityRand", "Random Intensity", 0, 0,
                                    1.0, 0.01));

    rotProps.add(this->addFloatProp("vector_influence",
                                    "Vector Rotation Influence", 1, 0, 1, 0.1));

    const scaleProps = this.createGroup("Scale");
    scaleProps.add(this->addEnumProp("sizeMode", "Size Mode",
                                     [ "Normal", "Keep Aspect" ]));
    scaleProps.add(this->addFloatProp("scale", "Scale", 1, 0, 4, 0.1));
    scaleProps.add(
        this->addFloatProp("scaleRand", "Scale random", 0, 0, 1, 0.1));

    auto source = R""""(

        mat3 transMat(vec2 t)
        {
            return mat3(vec3(1.0,0.0,0.0), vec3(0.0,1.0,0.0), vec3(t, 1.0));
        }

        mat3 scaleMat(vec2 s)
        {
            return mat3(vec3(s.x,0.0,0.0), vec3(0.0,s.y,0.0), vec3(0.0, 0.0, 1.0));
        }

        mat3 rotMat(float rot)
        {
            float r = radians(rot);
            return mat3(vec3(cos(r), -sin(r),0.0), vec3(sin(r), cos(r),0.0), vec3(0.0, 0.0, 1.0));
        }

        vec2 transformUV(vec2 uv, vec2 translate, float rot, vec2 scale)
        {
            mat3 trans = transMat(vec2(0.5, 0.5)) *
                transMat(vec2(translate.x, translate.y)) *
                rotMat(rot) *
                scaleMat(vec2(scale.x, scale.y)) *
                transMat(vec2(-0.5, -0.5));

            vec3 res = inverse(trans) * vec3(uv, 1.0);
            uv = res.xy;
            
            return clamp(uv,vec2(0.0), vec2(1.0));
        }

        float randomFloatRange(int index, float fmin, float fmax)
        {
            float r = _rand(vec2(_seed) + vec2(float(index) * 0.0001));

            return fmin + (fmax - fmin) * r;
        }

        // if uv is out of bounds then return vec4(0)
        vec4 sampleImage(sampler2D image, vec2 uv)
        {
            if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0)
                return texture(image, uv);
            return vec4(0.0);
        }

        vec4 blend(vec4 colA, vec4 colB)
        {
            vec4 col = vec4(1.0);
            if (prop_blendType==0) // max
                col.rgb = max(colA.rgb, colB.rgb);
            if (prop_blendType==1) // add
                col.rgb = colA.rgb + colB.rgb;

            return col;
        }

        vec2 calcScale()
        {
            float rowSpacing = 1.0 / float(prop_rows);
            float colSpacing = 1.0 / float(prop_columns);

            if (prop_sizeMode == 0)
            {
                return vec2(colSpacing, rowSpacing);
            }
            else if (prop_sizeMode == 1)
            {   
                return vec2(min(rowSpacing, colSpacing));
            }
            // else if (type == 2)
            // {
            //     // todo: absolute
            //     // let user specify size
            // }

            return vec2(1.0);
        }

        // https://stackoverflow.com/questions/38986208/webgl-loop-index-cannot-be-compared-with-non-constant-expression
        vec4 process(vec2 uv)
        {
            float rowSpacing = 1.0 / float(prop_rows);
            float colSpacing = 1.0 / float(prop_columns);

            vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
            for(int r = 0; r<prop_rows; r++)
                for(int c = 0; c<prop_columns; c++)
                {
                    int i = r * prop_rows + c;

                    // building transform for this tile
                    // transformUV transform the uv to the local space of
                    // this tile

                    // place x and y at the center of each tile
                    float x = float(c) * colSpacing + colSpacing * 0.5 - 0.5;
                    float y = float(r) * rowSpacing + rowSpacing * 0.5 - 0.5;

                    // random offset
                    float randPosX = randomFloatRange(i*17 + 4, -1.0, 1.0);
                    float randPosY = randomFloatRange(i*21 + 5, -1.0, 1.0);
                    vec2 randPos = normalize(vec2(randPosX, randPosY)) * prop_posRand;
                    x += randPos.x;
                    y += randPos.y;
                    
                    if (prop_offset_axis == 0) {
                        if (r % prop_offset_interval == 0)
                            x += colSpacing * prop_offset;
                    } else {
                        if (c % prop_offset_interval == 0)
                            y += rowSpacing * prop_offset;
                    }

                    float r = randomFloatRange(i*15 + 3, -180.0, 180.0) * prop_rotRand + prop_rot;

                    // intensity
                    float intens = 1.0;
                    // multiply by texture if available
                    if (intensity_connected) {
                        intens *= texture(intensity, vec2(x,y)).r;
                    }
                    // random intensity per tile
                    float randIntensity = randomFloatRange(i*21 + 8, 0.0, 1.0);
                    // lerp between random intensity and one by image ( or 1.0 )
                    intens = mix(intens, randIntensity, prop_intensityRand);

                    if (mask_connected) {
                        float mask = texture(mask, vec2(x,y) + vec2(0.5)).r;
                        if (mask < 0.001f)
                            continue;
                    }
                    
                    // scale starts out as the size of the tiles in uv space (0.0 - 1.0)
                    //float sx = colSpacing * prop_scale;
                    //float sy = rowSpacing * prop_scale;

                    vec2 scale = calcScale();
                    float sx = scale.x * prop_scale;
                    float sy = scale.y * prop_scale;

                    if (size_connected) {
                        float s = texture(size, vec2(x,y) + vec2(0.5)).r;
                        sx *= s;
                        sy *= s;
                    }
                    // random scale per tile
                    float randScale = randomFloatRange(i*27 + 3, 0.0, 1.0);
                    // lerp between random scale and one by image ( or 1.0 )
                    sx = mix(sx, randScale, prop_scaleRand);
                    sy = mix(sy, randScale, prop_scaleRand);

                    // vector
                    if (vector_connected) {
                        vec2 dir = (texture(vector, vec2(x,y) + vec2(0.5)).rg - 0.5) * 2.0;
                        r += degrees(atan(dir.x, dir.y)) * prop_vector_influence;
                    }

                    vec2 sampleUV = transformUV(uv, vec2(x,y), r, vec2(sx, sy));
                    color = blend(color, sampleImage(image, sampleUV) * intens);

                    // sample 4 sides
                    sampleUV = transformUV(uv, vec2(x,y) + vec2(-1.0,  0.0), r, vec2(sx, sy));
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0,  0.0), r, vec2(sx, sy));
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 0.0,  1.0), r, vec2(sx, sy));
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 0.0, -1.0), r, vec2(sx, sy));
                    color = blend(color, sampleImage(image, sampleUV) * intens);

                    // sample 4 diagonal sides
                    sampleUV = transformUV(uv, vec2(x,y) + vec2(-1.0,  1.0), r, vec2(sx, sy));
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0,  1.0), r, vec2(sx, sy));
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0,  1.0), r, vec2(sx, sy));
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0, -1.0), r, vec2(sx, sy));
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                }

            return color;
        }
        )"""";

    this->setShaderSource(source);
}
