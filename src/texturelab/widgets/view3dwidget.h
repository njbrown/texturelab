#pragma once

#include <QFrame>
#include <QMainWindow>

class Viewer3D;
class View3DWidget : public QMainWindow {
public:
    View3DWidget();
    Viewer3D* viewer;

    void reRender();
};