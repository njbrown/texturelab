#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

RgbaMergeNode::init()
{
    this->title = "RGBA Merge";

    this->addInput("redSource");
    this->addInput("greenSource");
    this->addInput("blueSource");
    this->addInput("alphaSource");

    // Red
    let prop =
        this->addEnumProp("redSource", "Red Source",
                          [ "Red", "Green", "Blue", "Alpha", "Average (RGB)" ]);
    prop.setValue(0);

    // Green
    prop =
        this->addEnumProp("greenSource", "Green Source",
                          [ "Red", "Green", "Blue", "Alpha", "Average (RGB)" ]);
    prop.setValue(1);

    // Blue
    prop =
        this->addEnumProp("blueSource", "Blue Source",
                          [ "Red", "Green", "Blue", "Alpha", "Average (RGB)" ]);
    prop.setValue(2);

    // Alpha
    prop =
        this->addEnumProp("alphaSource", "Alpha Source",
                          [ "Red", "Green", "Blue", "Alpha", "Average (RGB)" ]);
    prop.setValue(3);

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
            vec4 r = texture(redSource,uv);
            vec4 g = texture(greenSource,uv);
            vec4 b = texture(blueSource,uv);
            vec4 a = texture(alphaSource,uv);

			vec4 result = vec4(0, 0, 0, 1);
			if (redSource_connected)
				result.r = getChannel(r, prop_redSource);
			if (greenSource_connected)
				result.g = getChannel(g, prop_greenSource);
			if (blueSource_connected)
				result.b = getChannel(b, prop_blueSource);
			if (alphaSource_connected)
				result.a = getChannel(a, prop_alphaSource);
            
            return result;
        }
          )"""";

    this->setShaderSource(source);
}