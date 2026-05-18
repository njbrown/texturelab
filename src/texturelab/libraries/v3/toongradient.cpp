#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

// Toon / posterised gradient — remaps greyscale input to a stepped,
// band-mapped gradient.  Each band maps to a colour stop in the gradient
// prop, giving crisp toon-shading zones while preserving artist colour
// control.  Critical for R&C-style roughness (3-band) and albedo (4-6 band)
// maps that read as stylised even up close.
void ToonGradientNode::init()
{
    this->title = "Toon Gradient";

    this->addInput("image");

    this->addIntProp  ("bands",    "Bands",        4,   2,  16, 1);
    this->addFloatProp("softness", "Edge Softness", 0.05, 0.0, 0.5, 0.01);
    this->addGradientProp("gradient", "Gradient", Gradient::defaultGradient());

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            if (!image_connected)
                return vec4(0.0, 0.0, 0.0, 1.0);

            float input = texture(image, uv).r;
            float bands = float(prop_bands);

            // Quantise to N bands
            float bandIdx   = floor(input * bands);
            float bandFract = fract(input * bands);

            // Soft transition between bands using smoothstep on the intra-band fraction
            float soft = smoothstep(0.0, prop_softness * bands, bandFract)
                       * (1.0 - smoothstep((1.0 - prop_softness) * bands,
                                            bands, bandFract + bandIdx * 1.0));

            // Map to [0,1] for gradient sampling
            float t = (bandIdx + clamp(soft, 0.0, 1.0)) / bands;
            t = clamp(t, 0.0, 1.0);

            vec3 col = sampleGradient(prop_gradient, t);
            return vec4(col, 1.0);
        }
    )"""";

    this->setShaderSource(source);
}
