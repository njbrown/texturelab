#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

FloodFillNode::init()
{
    auto source = R""""(
		)"""";
    this->setShaderSource(source);
}