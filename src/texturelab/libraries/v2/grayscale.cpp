#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

GrayscaleNode::init()
{
    this->title = "Grayscale";

    this->addInput("image");

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            // grayscale input color
            vec4 col = texture(image, uv);
			
			col.rgb = vec3((col.r + col.g + col.b) * 0.3333333);

            return col;
        }
          )"""";

    this->setShaderSource(source);
}
