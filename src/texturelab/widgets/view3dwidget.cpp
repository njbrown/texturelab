#include "view3dwidget.h"
#include "viewer3d.h"

View3DWidget::View3DWidget()
{
    this->viewer = new Viewer3D();
    this->setCentralWidget(viewer);
}

void View3DWidget::reRender() { this->viewer->reRender(); }