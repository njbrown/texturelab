#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

ImageNode::init()
{
    this->title = "Image";

    this->imageProp = this->addImageProperty("image", "Image");
}