#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

RgbaShuffleNode::init()
{
    this->title = "RGBA Shuffle";

    this->addInput("inputImage");

    // Red
    let prop =
        this->addEnumProp("redSource", "Red Source",
                          [ "Red", "Green", "Blue", "Alpha", "Average (RGB)" ]);
    prop->setValue(0);

    // Green
    prop =
        this->addEnumProp("greenSource", "Green Source",
                          [ "Red", "Green", "Blue", "Alpha", "Average (RGB)" ]);
    prop->setValue(1);

    // Blue
    prop =
        this->addEnumProp("blueSource", "Blue Source",
                          [ "Red", "Green", "Blue", "Alpha", "Average (RGB)" ]);
    prop->setValue(2);

    // Alpha
    prop =
        this->addEnumProp("alphaSource", "Alpha Source",
                          [ "Red", "Green", "Blue", "Alpha", "Average (RGB)" ]);
    prop->setValue(3);

    auto source = R""""(

		float getChannel(vec4 inputData, int mode)
		{
			if (mode == 0) return inputData.r;
			if (mode == 1) return inputData.g;
			if (mode == 2) return inputData.b;
			if (mode == 3) return inputData.a;
			if (mode == 4) {
				return (inputData.r + inputData.g + inputData.b) * 0.3333333;
			}

			return 0.0;
		}

        vec4 process(vec2 uv)
        {
            // grayscale input color
            vec4 col = texture(inputImage,uv);

			vec4 result = vec4(0);
			result.r = getChannel(col, prop_redSource);
			result.g = getChannel(col, prop_greenSource);
			result.b = getChannel(col, prop_blueSource);
			result.a = getChannel(col, prop_alphaSource);
            
            return result;
        }
          )"""";

    this->setShaderSource(source);
}
