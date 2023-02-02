#include "propertieswidget.h"
#include "../../models.h"
#include "../../props.h"
#include "propwidgets.h"

#include <QVBoxLayout>

PropertiesWidget::PropertiesWidget() : QWidget()
{
    displayMode = PropertyDisplayMode::None;

    textureChannelProp = new EnumProp();
    textureChannelProp->displayName = "Texture Channel";
    textureChannelProp->values = {"None",      "Albedo", "Normal", "Metalness",
                                  "Roughness", "Height", "Alpha"};
    textureChannelProp->setValue(0);

    auto layout = new QVBoxLayout(this);
    layout->addStretch(1);
    this->setLayout(layout);
}

void PropertiesWidget::setSelectedNode(const TextureNodePtr& node)
{
    qDebug() << "Displaying properties for node: " << node->title;
    this->selectedNode = node;

    // clear current properties
    this->clearSelection();

    auto layout = (QVBoxLayout*)this->layout();

    // add base props
    this->addBasePropsToLayout();

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

void PropertiesWidget::addBasePropsToLayout()
{
    auto layout = (QVBoxLayout*)this->layout();

    // texture channel
    auto widget = new EnumPropWidget();

    // determine value of prop
    auto& channels = this->project->textureChannels;
    int channelVal = 0;
    for (auto key : channels.keys()) {
        if (channels[key] == selectedNode->id) {
            channelVal = (int)key;
            break;
        }
    }
    textureChannelProp->setValue(channelVal);

    widget->setProp(textureChannelProp);
    propWidgets.append(widget);

    connect(widget, &EnumPropWidget::valueChanged, [=](int value) {
        emit this->textureChannelUpdated((TextureChannel)value,
                                         this->selectedNode);
    });
    layout->addWidget(widget);

    // todo: random seed
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

    // remove all other items (todo: delete them)
    while (layout->count() > 0) {
        auto item = layout->itemAt(0);
        layout->removeItem(item);

        auto widget = item->widget();
        if (widget)
            widget->deleteLater();
    }

    propWidgets.clear();
}

void PropertiesWidget::setProject(const TextureProjectPtr& project)
{
    this->project = project;
}