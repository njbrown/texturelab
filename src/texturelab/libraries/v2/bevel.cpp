#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

BevelNode::init()
{
    auto source = R""""(
		)"""";
    this->setShaderSource(source);
}