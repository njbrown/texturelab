#include "viewer3d.h"
#include <QApplication>
#include <QMainWindow>
#include <QStackedLayout>

#include <QOpenGLContext>
#include <QOpenGLWindow>
#include <QSurfaceFormat>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    QMainWindow w;

    // init gl
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);

    // Request OpenGL 3.3 core or OpenGL ES 3.0.
    // if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) {
    qDebug("Requesting 3.3 core context");
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    // }
    // else {
    //     qDebug("Requesting 3.0 context");
    //     fmt.setVersion(3, 0);
    // }

    QSurfaceFormat::setDefaultFormat(fmt);

    auto viewer = new Viewer3D();

    w.setCentralWidget(viewer);
    w.resize(800, 600);

    w.show();
    w.showMaximized();

    return a.exec();
}
