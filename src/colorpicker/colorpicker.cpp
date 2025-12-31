#include "colorpicker.h"
#include "./widgets.h"
#include <QDialogButtonBox>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

ColorPicker::ColorPicker()
{
    svBox = new SVBox();
    hueSlider = new HueSlider();
    // alphaSlider = new AlphaSlider();

    svBox->setColor(QColor(255, 150, 0, 255));
    originalColor = QColor(255, 150, 0, 255);

    // connect widget signals to emit color changes
    connect(svBox, &SVBox::onSVChanged, this, [this](float s, float v) {
        QColor currentColor = svBox->getColor();
        emit onColorChanged(currentColor);
    });
    connect(hueSlider, &HueSlider::onHueChanged, this, [this](float h) {
        QColor currentColor = svBox->getColor();
        currentColor.setHsvF(h, currentColor.saturationF(),
                             currentColor.valueF());
        svBox->setColor(currentColor);
        emit onColorChanged(currentColor);
    });

    // add layout
    auto vlayout = new QVBoxLayout(this);
    vlayout->addWidget(svBox);
    vlayout->addWidget(hueSlider);
    // vlayout->addWidget(alphaSlider);

    // add OK and Cancel buttons
    auto buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, [this]() {
        // Revert to original color on cancel
        svBox->setColor(originalColor);
        hueSlider->setColor(originalColor);
        emit onColorChanged(originalColor);
        QDialog::reject();
    });
    vlayout->addWidget(buttonBox);

    this->setLayout(vlayout);

    // this->setBaseSize(400, 500);
    this->resize(400, 330);
}

void ColorPicker::setColor(const QColor& color)
{
    originalColor = color;
    svBox->setColor(color);
    hueSlider->setColor(color);
    // alphaSlider->setColor(color);
}