#include "mainwindow.h"

#include <QApplication>
#include <QSurfaceFormat>

// Hints that a dedicated GPU should be used whenever possible
// https://stackoverflow.com/a/39047129/991834
#ifdef Q_OS_WIN
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main(int argc, char* argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 2);
    // format.setColorSpace(QSurfaceFormat::sRGBColorSpace);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    MainWindow w;

    w.show();
    w.showMaximized();

    return a.exec();
}
