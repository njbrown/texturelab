#include "gradientpicker.h"
#include "gradient.h"

GradientSlider::GradientSlider() { this->initUI(); }

void GradientSlider::setGradient(const Gradient& gradient)
{
    this->gradient = gradient;
    this->update();
}

void GradientSlider::initUI() { this->setFixedSize(400, 100); }

GradientPickerDialog::GradientPickerDialog() { this->initUI(); }

void GradientPickerDialog::setGradient(const Gradient& gradient)
{
    // this->gradient = gradient;
    this->gradientSlider->setGradient(gradient);

    // update ui
    // this->update();
}

void GradientPickerDialog::initUI()
{
    // create ui
    // this->setFixedSize(400, 100);
}