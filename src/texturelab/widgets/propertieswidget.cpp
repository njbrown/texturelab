#include "propertieswidget.h"
#include "../models.h"
#include "../props.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

const int SLIDER_MAX = 1000;

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

PropertiesWidget::PropertiesWidget() : QWidget()
{
    displayMode = PropertyDisplayMode::None;

    auto layout = new QVBoxLayout(this);
    layout->addStretch(1);
    this->setLayout(layout);
}

void PropertiesWidget::setSelectedNode(const TextureNodePtr& node)
{
    qDebug() << "Displaying properties for node: " << node->title;

    // clear current properties
    this->clearSelection();

    auto layout = (QVBoxLayout*)this->layout();

    // add new props to layout
    for (auto prop : node->props) {
        switch (prop->type) {
        case PropType::Float: {
            auto widget = new FloatPropWidget();
            widget->setProp((FloatProp*)prop);
            propWidgets.append(widget);

            connect(widget, &FloatPropWidget::valueChanged, [=](double value) {
                qDebug() << "prop" << prop->name << " changed: " << value;
                // set node value

                node->setProp(prop->name, value);
                // mark subsequent nodes as dirty!!!

                project->markNodeAsDirty(node);

                emit propertyUpdated(prop->name, value);
            });
            layout->addWidget(widget);

        } break;
        }
    }

    layout->addStretch(1);
}

void PropertiesWidget::clearSelection()
{
    displayMode = PropertyDisplayMode::None;

    auto layout = this->layout();

    // clear widget
    for (auto widget : propWidgets) {
        widget->hide();
        widget->deleteLater();
    }

    // remove all items
    while (layout->count() > 0)
        layout->removeItem(layout->itemAt(0));

    propWidgets.clear();
}

void PropertiesWidget::setProject(const TextureProjectPtr& project)
{
    this->project = project;
}