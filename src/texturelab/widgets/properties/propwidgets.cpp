#include "propwidgets.h"
#include "../../models.h"
#include "../../props.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

const int SLIDER_MAX = 1000;

// FLOAT PROP WIDGET
// https://stackoverflow.com/a/19007951
FloatPropWidget::FloatPropWidget()
{
    prop = nullptr;

    auto vlayout = new QVBoxLayout(this);
    this->setLayout(vlayout);

    // label
    label = new QLabel(this);
    label->setText("");
    vlayout->addWidget(label);

    // slider
    slider = new QSlider(Qt::Horizontal, this);
    slider->setMinimum(0);
    slider->setMaximum(SLIDER_MAX);
    slider->setSingleStep(1);

    spinbox = new QDoubleSpinBox(this);

    auto hbox = new QHBoxLayout();
    hbox->addWidget(slider);
    hbox->addWidget(spinbox);

    vlayout->addLayout(hbox);

    this->setFixedHeight(80);

    connect(slider, &QSlider::valueChanged, [=](int val) {
        auto percent = val / (float)SLIDER_MAX;
        if (prop) {
            auto range = prop->maxValue - prop->minValue;
            auto finalValue = prop->minValue + range * percent;
            spinbox->setValue(finalValue);

            emit valueChanged(finalValue);
        }
    });

    connect(spinbox, &QDoubleSpinBox::valueChanged, [=](double val) {
        if (prop) {
            auto range = prop->maxValue - prop->minValue;
            auto finalValue = (val / range) * SLIDER_MAX;

            slider->setValue(finalValue);

            emit valueChanged(val);
        }
    });
}

void FloatPropWidget::setProp(FloatProp* prop)
{
    label->setText(prop->displayName);

    spinbox->setMinimum(prop->minValue);
    spinbox->setMaximum(prop->maxValue);
    spinbox->setSingleStep(prop->step);
    spinbox->setValue(prop->value);

    auto range = prop->maxValue - prop->minValue;
    auto finalValue = (prop->value / range) * SLIDER_MAX;

    slider->setValue(finalValue);

    this->prop = prop;
}

// INT PROP WIDGET
// https://stackoverflow.com/a/19007951
IntPropWidget::IntPropWidget()
{
    prop = nullptr;

    auto vlayout = new QVBoxLayout(this);
    this->setLayout(vlayout);

    // label
    label = new QLabel(this);
    label->setText("");
    vlayout->addWidget(label);

    // slider
    slider = new QSlider(Qt::Horizontal, this);
    slider->setMinimum(0);
    slider->setMaximum(SLIDER_MAX);
    slider->setSingleStep(1);

    spinbox = new QSpinBox(this);

    auto hbox = new QHBoxLayout();
    hbox->addWidget(slider);
    hbox->addWidget(spinbox);

    vlayout->addLayout(hbox);

    this->setFixedHeight(80);

    connect(slider, &QSlider::valueChanged, [=](int val) {
        auto percent = val / (float)SLIDER_MAX;
        if (prop) {
            spinbox->setValue(val);

            emit valueChanged(val);
        }
    });

    connect(spinbox, &QSpinBox::valueChanged, [=](int val) {
        if (prop) {
            slider->setValue(val);

            emit valueChanged(val);
        }
    });
}

void IntPropWidget::setProp(IntProp* prop)
{
    label->setText(prop->displayName);

    spinbox->setMinimum(prop->minValue);
    spinbox->setMaximum(prop->maxValue);
    spinbox->setSingleStep(prop->step);
    spinbox->setValue(prop->value);

    slider->setValue(prop->value);
    slider->setMinimum(prop->minValue);
    slider->setMaximum(prop->maxValue);
    slider->setSingleStep(prop->step);

    this->prop = prop;
}

// ENUM PROP WIDGET
// https://stackoverflow.com/a/19007951
EnumPropWidget::EnumPropWidget()
{
    prop = nullptr;

    auto vlayout = new QVBoxLayout(this);
    this->setLayout(vlayout);

    // label
    label = new QLabel(this);
    label->setText("");
    vlayout->addWidget(label);

    // slider
    comboBox = new QComboBox(this);
    vlayout->addWidget(comboBox);

    this->setFixedHeight(80);

    connect(comboBox, &QComboBox::currentIndexChanged,
            [=](int val) { emit valueChanged(val); });
}

void EnumPropWidget::setProp(EnumProp* prop)
{
    label->setText(prop->displayName);

    for (auto item : prop->values) {
        comboBox->addItem(item);
    }

    comboBox->setCurrentIndex(prop->index);

    this->prop = prop;
}

// BOOL PROP WIDGET
// https://stackoverflow.com/a/19007951
BoolPropWidget::BoolPropWidget()
{
    prop = nullptr;

    auto vlayout = new QVBoxLayout(this);
    this->setLayout(vlayout);

    // label
    label = new QLabel(this);
    label->setText("");
    vlayout->addWidget(label);

    // slider
    button = new QPushButton(this);
    vlayout->addWidget(button);

    this->setFixedHeight(80);

    connect(button, &QPushButton::pressed, [=]() {
        setValue(!this->value);
        emit valueChanged(this->value);
    });
}

void BoolPropWidget::setProp(BoolProp* prop)
{
    label->setText(prop->displayName);

    setValue(prop->value);
}

void BoolPropWidget::setValue(bool value)
{
    this->value = value;
    if (value) {
        button->setText("True");
    }
    else {
        button->setText("False");
    }
}