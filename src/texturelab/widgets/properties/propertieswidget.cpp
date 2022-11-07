#include "propertieswidget.h"
#include "../../models.h"
#include "../../props.h"
#include "propwidgets.h"

#include <QVBoxLayout>

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
                // qDebug() << "prop" << prop->name << " changed: " << value;
                // set node value

                node->setProp(prop->name, value);
                project->markNodeAsDirty(node);

                emit propertyUpdated(prop->name, value);
            });
            layout->addWidget(widget);

        } break;
        case PropType::Int: {
            auto widget = new IntPropWidget();
            widget->setProp((IntProp*)prop);
            propWidgets.append(widget);

            connect(widget, &IntPropWidget::valueChanged, [=](int value) {
                // qDebug() << "prop" << prop->name << " changed: " << value;
                // set node value

                node->setProp(prop->name, value);
                project->markNodeAsDirty(node);

                emit propertyUpdated(prop->name, value);
            });
            layout->addWidget(widget);

        } break;
        case PropType::Enum: {
            auto widget = new EnumPropWidget();
            widget->setProp((EnumProp*)prop);
            propWidgets.append(widget);

            connect(widget, &EnumPropWidget::valueChanged, [=](int value) {
                node->setProp(prop->name, value);
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