#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"

void ImageNode::init()
{
    this->title = "Image";

    this->imageProp = this->addImageProp("image", "Image");
}