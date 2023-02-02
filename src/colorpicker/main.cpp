#include "colorpicker.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    ColorPicker dialog;

    dialog.resize(800, 600);
    dialog.show();

    return a.exec();
}
