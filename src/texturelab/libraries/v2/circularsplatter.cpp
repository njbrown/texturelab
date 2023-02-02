#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

void CircularSplatterNode::init()
{
    this->title = "Circular Splatter";

    this->addInput("image");
    this->addInput("mask");
    this->addInput("size");
    this->addInput("intensity");

    this->addIntProp("count", "Count", 10, 0, 50, 1);
    this->addIntProp("rings", "Rings", 1, 0, 5, 1);

    this->addEnumProp("blendType", "Blend Type", {"Max", "Add"});

    this->addFloatProp("radius", "Radius", 0.3, 0, 1.0, 0.01);
    this->addFloatProp("spacing", "Spacing", 1.0, 0, 2.0, 0.01);
    this->addFloatProp("spiralInfluence", "Spiral Influence", 0.0, 0.0, 1.0,
                       0.01);
    this->addBoolProp("reverseSpiral", "Reverse Spiral Direction", false);

    // ROTATION
    auto rotProp = this->createGroup("Rotation");
    rotProp->collapsed = false;
    rotProp->add(this->addFloatProp("rot", "Rotation", 0, 0, 360, 0.1));
    rotProp->add(
        this->addFloatProp("rotRand", "Random Rotation", 0.0, 0.0, 1.0, 0.01));
    rotProp->add(
        this->addFloatProp("ringRot", "Ring Rotation", 0, 0, 360, 0.1));
    rotProp->add(this->addFloatProp("ringRotRand", "Ring Rotation Random", 0.0,
                                    0.0, 1.0, 0.01));
    rotProp->add(this->addFloatProp("ringRotOffset", "Ring Rotation Offset",
                                    0.0, 0.0, 1.0, 0.01));
    rotProp->add(this->addBoolProp("pivotCenter",
                                   "Pivot Orientation From Center", true));

    // INTENSITY
    auto intensityProp = this->createGroup("Intensity");
    intensityProp->collapsed = false;
    intensityProp->add(this->addFloatProp("intensityRand", "Random Intensity",
                                          0, 0, 1.0, 0.01));
    intensityProp->add(this->addFloatProp(
        "intensityByRing", "Intensity By Ring", 0, 0, 1.0, 0.01));
    intensityProp->add(this->addBoolProp("invertIntensityByRing",
                                         "Invert Intensity By Ring", true));
    intensityProp->add(this->addFloatProp(
        "intensityByAngle", "Intensity By Angle", 0, 0, 1.0, 0.01));
    intensityProp->add(this->addBoolProp("invertIntensityByAngle",
                                         "Invert Intensity By Angle", true));

    // SCALE
    auto scaleProp = this->createGroup("Scale");
    scaleProp->collapsed = false;
    scaleProp->add(
        this->addFloatProp("inputSize", "Input Size", 0.1, 0, 1, 0.01));
    scaleProp->add(this->addFloatProp("scale", "Scale", 1, 0, 1, 0.01));
    scaleProp->add(
        this->addFloatProp("scaleRand", "Scale random", 0, 0, 1, 0.01));

    scaleProp->add(
        this->addFloatProp("scaleByRing", "Scale By Ring", 0, 0, 1.0, 0.01));
    scaleProp->add(this->addBoolProp("invertScaleByRing",
                                     "Scale Intensity By Ring", true));
    scaleProp->add(
        this->addFloatProp("scaleByAngle", "Scale By Angle", 0, 0, 1.0, 0.01));
    scaleProp->add(this->addBoolProp("invertScaleByAngle",
                                     "Scale Intensity By Angle", true));

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

        float randomFloatRange(vec2 seed, float fmin, float fmax)
        {
            float r = _rand(vec2(_seed) + vec2(seed * 0.0001));

            return fmin + (fmax - fmin) * r;
        }

        // if uv is out of bounds then return vec4(0)
        vec4 sampleImage(sampler2D image, vec2 uv)
        {
            if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0)
                return texture(image, uv);
            return vec4(0.0);
        }

        float calcIntensity(int i, vec2 uv, float radialFactor, float invRadialFactor, float angleFactor)
		{
			// intensity
			float intens = 1.0;
			// multiply by texture if available
			if (intensity_connected) {
				intens *= texture(intensity, uv).r;
			}
			// random intensity per tile
			// float randIntensity = randomFloatRange(uv, 0.0, 1.0);
			float randIntensity = randomFloatRange(i*13 + 9, 0.0, 1.0);

			// lerp between random intensity and one by image ( or 1.0 )
            intens = mix(intens, randIntensity, prop_intensityRand);

            // multiply by radial factor
            float intensityRadialFactor = 1.0;
            if (prop_invertIntensityByRing)
                intensityRadialFactor = mix(1.0, invRadialFactor, prop_intensityByRing);
            else
                intensityRadialFactor = mix(1.0, radialFactor, prop_intensityByRing);

            intens *= intensityRadialFactor;

            // multiply by angle factor
            float intensityAngleFactor = 1.0;
            if (prop_invertIntensityByAngle)
                intensityAngleFactor = mix(1.0, (1.0 - angleFactor), prop_intensityByAngle);
            else
                intensityAngleFactor = mix(1.0, angleFactor, prop_intensityByAngle);

            intens *= intensityAngleFactor;

			return intens;
        }
        
        vec2 calcScale(int i, vec2 uv, float radialFactor, float invRadialFactor, float angleFactor)
		{
			float s = prop_scale;
			if (size_connected) {
				s *= texture(size, uv).r;
			}

            float randScale = randomFloatRange(i*11 + 6, 0.0, 1.0);
            //float randScale = randomFloatRange(uv, 0.0, 1.0);
            
            s = mix(s, randScale, prop_scaleRand);
            
            // multiply by radial factor
            float scaleRadialFactor = 1.0;
            if (prop_invertScaleByRing)
                scaleRadialFactor = mix(1.0, invRadialFactor, prop_scaleByRing);
            else
                scaleRadialFactor = mix(1.0, radialFactor, prop_scaleByRing);

            s *= scaleRadialFactor;

            // multiply by angle factor
            float scaleAngleFactor = 1.0;
            if (prop_invertScaleByAngle)
                scaleAngleFactor = mix(1.0, (1.0 - angleFactor), prop_scaleByAngle);
            else
                scaleAngleFactor = mix(1.0, angleFactor, prop_scaleByAngle);

            s *= scaleAngleFactor;

			return vec2(s) * prop_inputSize;
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

        // https://stackoverflow.com/questions/38986208/webgl-loop-index-cannot-be-compared-with-non-constant-expression
        vec4 process(vec2 uv)
        {
            vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
            float spacing = prop_radius/float(prop_rings);
            float angle_spacing = 360.0/float(prop_count);

            // calculate rings from the outside going in
            for(int ring = 0; ring<prop_rings; ring++)
            {
                float radius = prop_radius - spacing * float(ring);
                float ringRandomAngle = mix(0.0, randomFloatRange(ring*27 + 9, 0.0, 360.0), prop_ringRotRand);


                for(int i = 0; i<prop_count; i++)
                {
                    int ringId = ring * prop_count + i;

                    float radialFactor = float(ring + 1) / float(prop_rings); // how close to the center are we
                    float invRadialFactor = 1.0 - float(ring) / float(prop_rings);

                    float angleFactor = (angle_spacing * float(i)) / 360.0;

                    float spiralFactor = angleFactor;
                    if (prop_reverseSpiral)
                        spiralFactor = 1.0 - spiralFactor;

                    // angle offset per ring
                    float angleOffset = angle_spacing * prop_ringRotOffset * float(prop_rings - ring - 1);
                    
                    // todo: check the effects of prop_ringRot
                    float angle = angle_spacing * 
                                  prop_spacing * 
                                  float(i) + 
                                  prop_ringRot + 
                                  ringRandomAngle + 
                                  angleOffset;

                    float finalRadius = mix(radius, radius * spiralFactor, prop_spiralInfluence);
                    float x = cos(radians(angle)) * finalRadius;
                    float y = sin(radians(angle)) * finalRadius;
                    float r = 0.0;
                    
                    if (prop_pivotCenter)
                        r = 360.0 - angle + 90.0;
                    r += prop_rot;

                    float randRot = mix(0.0, randomFloatRange(ringId*31 + 7, 0.0, 360.0), prop_rotRand);
                    r += randRot;

                    vec2 randomId = vec2(cos(radians(angle)) * finalRadius,
                                        sin(radians(angle)) * finalRadius);

                    if (mask_connected) {
                        float mask = texture(mask, vec2(x,y) + vec2(0.5)).r;
                        if (mask < 0.001f)
                            continue;
                    }

                    vec2 s = calcScale(ringId, vec2(x,y) + vec2(0.5), radialFactor, invRadialFactor, angleFactor);

                    float intens = calcIntensity(ringId, vec2(x,y) + vec2(0.5), radialFactor, invRadialFactor, angleFactor);

                    vec2 sampleUV = transformUV(uv, vec2(x,y), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);

                    // sample 4 sides
                    sampleUV = transformUV(uv, vec2(x,y) + vec2(-1.0,  0.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0,  0.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 0.0,  1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 0.0, -1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);

                    // sample 4 diagonal sides
                    sampleUV = transformUV(uv, vec2(x,y) + vec2(-1.0,  1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0,  1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0,  1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                    sampleUV = transformUV(uv, vec2(x,y) + vec2( 1.0, -1.0), r, s);
                    color = blend(color, sampleImage(image, sampleUV) * intens);
                }
            }

            //color = color / vec4(float(prop_count));

            return color;
        }
        )"""";

    this->setShaderSource(source);
}
