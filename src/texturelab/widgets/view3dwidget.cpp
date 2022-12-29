#include "view3dwidget.h"
#include "viewer3d.h"

View3DWidget::View3DWidget()
{
    this->viewer = new Viewer3D();
    this->setCentralWidget(viewer);

    this->viewer->setDefaultEnvironment(":env/cave_wall_1k.hdr");
}

void View3DWidget::reRender() { this->viewer->reRender(); }