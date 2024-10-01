#include "colorpicker.h"
#include "./widgets.h"
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

ColorPicker::ColorPicker()
{
    svBox = new SVBox();
    hueSlider = new HueSlider();
    // alphaSlider = new AlphaSlider();

    svBox->setColor(QColor(255, 150, 0, 255));

    // add layout
    auto vlayout = new QVBoxLayout(this);
    vlayout->addWidget(svBox);
    vlayout->addWidget(hueSlider);
    // vlayout->addWidget(alphaSlider);

    this->setLayout(vlayout);

    // this->setBaseSize(400, 500);
    this->resize(400, 330);
}

void ColorPicker::setColor(const QColor& color)
{
    svBox->setColor(color);
    hueSlider->setColor(color);
    // alphaSlider->setColor(color);
}