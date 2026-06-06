#include "propertieswidget.h"
#include "../../models.h"
#include "../../props.h"
#include "curvepropwidget.h"
#include "propwidgets.h"

#include <QVBoxLayout>
#include <QLabel>

PropertiesWidget::PropertiesWidget() : QWidget()
{
    displayMode = PropertyDisplayMode::None;

    textureChannelProp = new EnumProp();
    textureChannelProp->displayName = "Texture Channel";
    textureChannelProp->values = {"None",      "Albedo", "Normal", "Metalness",
                                  "Roughness", "Height", "Alpha", "AO"};
    textureChannelProp->setValue(0);

    randomSeedProp = new IntProp();
    randomSeedProp->displayName = "Random Seed";
    randomSeedProp->minValue = 0;
    randomSeedProp->maxValue = 50;
    randomSeedProp->step = 1;
    randomSeedProp->setValue(0);

    auto layout = new QVBoxLayout(this);
    layout->addStretch(1);
    this->setLayout(layout);
}

void PropertiesWidget::setSelectedNode(const TextureNodePtr& node)
{
    qDebug() << "Displaying properties for node: " << node->title;

    // clear current properties first, then assign (clearSelection resets selectedNode)
    this->clearSelection();
    this->selectedNode = node;

    auto layout = (QVBoxLayout*)this->layout();

    // add base props
    this->addBasePropsToLayout();

    // sort props by order
    QList<Prop*> sortedProps = node->props.values();
    std::sort(sortedProps.begin(), sortedProps.end(),
              [](Prop* a, Prop* b) { return a->order < b->order; });

    // add new props to layout
    for (auto prop : sortedProps) {
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
        case PropType::Bool: {
            auto widget = new BoolPropWidget();
            widget->setProp((BoolProp*)prop);
            propWidgets.append(widget);

            connect(widget, &BoolPropWidget::valueChanged, [=](bool value) {
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
        case PropType::Color: {
            auto widget = new ColorPropWidget();
            widget->setProp((ColorProp*)prop);
            propWidgets.append(widget);

            connect(widget, &ColorPropWidget::valueChanged,
                    [=](const QColor& value) {
                        node->setProp(prop->name, value);
                        project->markNodeAsDirty(node);

                        emit propertyUpdated(prop->name, value);
                    });
            layout->addWidget(widget);

        } break;
        case PropType::Gradient: {
            auto widget = new GradientPropWidget();
            widget->setProp((GradientProp*)prop);
            propWidgets.append(widget);

            connect(widget, &GradientPropWidget::valueChanged,
                    [=](const Gradient& value) {
                        node->setProp(prop->name, QVariant::fromValue(value));
                        project->markNodeAsDirty(node);

                        emit propertyUpdated(prop->name,
                                             QVariant::fromValue(value));
                    });
            layout->addWidget(widget);

        } break;
        case PropType::Image: {
            auto widget = new ImagePropWidget();
            widget->setProp((ImageProp*)prop);
            propWidgets.append(widget);

            connect(widget, &ImagePropWidget::valueChanged,
                    [=](const QImage& value) {
                        node->setProp(prop->name, value);
                        project->markNodeAsDirty(node);

                        emit propertyUpdated(prop->name, value);
                    });
            layout->addWidget(widget);

        } break;
        case PropType::String: {
            auto widget = new StringPropWidget();
            widget->setProp((StringProp*)prop);
            propWidgets.append(widget);

            connect(widget, &StringPropWidget::valueChanged,
                    [=](const QString& value) {
                        node->setProp(prop->name, value);
                        project->markNodeAsDirty(node);

                        emit propertyUpdated(prop->name, value);
                    });
            layout->addWidget(widget);

        } break;
        case PropType::Curve: {
            auto widget = new CurvePropWidget((CurveProp*)prop);
            propWidgets.append(widget);

            connect(widget, &CurvePropWidget::valueChanged,
                    [=](const Curve& value) {
                        node->setProp(prop->name, QVariant::fromValue(value));
                        project->markNodeAsDirty(node);

                        emit propertyUpdated(prop->name,
                                             QVariant::fromValue(value));
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

    randomSeedProp->setValue((int)this->selectedNode->randomSeed);

    auto seedWidget = new IntPropWidget();
    seedWidget->setProp(randomSeedProp);
    connect(seedWidget, &IntPropWidget::valueChanged, [=](int value) {
        this->selectedNode->randomSeed = value;
        this->project->markNodeAsDirty(this->selectedNode);

        emit this->propertyUpdated("randomSeed", value);
    });
    layout->addWidget(seedWidget);
}

void PropertiesWidget::setSelectedFrame(const FramePtr& frame)
{
    this->clearSelection();
    if (!frame)
        return;

    this->selectedFrame = frame;
    displayMode = PropertyDisplayMode::Frame;

    auto layout = (QVBoxLayout*)this->layout();

    auto titleLabel = new QLabel("Frame");
    titleLabel->setStyleSheet("font-weight: bold; margin-bottom: 4px;");
    layout->addWidget(titleLabel);

    auto titleProp = new StringProp();
    titleProp->displayName = "Title";
    titleProp->value = frame->text;
    auto titleWidget = new StringPropWidget();
    titleWidget->setProp(titleProp);
    propWidgets.append(titleWidget);

    connect(titleWidget, &StringPropWidget::valueChanged, [=](const QString& value) {
        frame->text = value;
        emit framePropertyChanged(frame);
    });
    layout->addWidget(titleWidget);

    auto colorProp = new ColorProp();
    colorProp->displayName = "Color";
    colorProp->value = frame->color;
    auto colorWidget = new ColorPropWidget();
    colorWidget->setProp(colorProp);
    propWidgets.append(colorWidget);

    connect(colorWidget, &ColorPropWidget::valueChanged, [=](const QColor& color) {
        frame->color = color;
        emit framePropertyChanged(frame);
    });
    layout->addWidget(colorWidget);

    layout->addStretch(1);
}

void PropertiesWidget::setSelectedComment(const CommentPtr& comment)
{
    this->clearSelection();
    if (!comment)
        return;

    this->selectedComment = comment;
    displayMode = PropertyDisplayMode::Comment;

    auto layout = (QVBoxLayout*)this->layout();

    auto titleLabel = new QLabel("Comment");
    titleLabel->setStyleSheet("font-weight: bold; margin-bottom: 4px;");
    layout->addWidget(titleLabel);

    auto textProp = new StringProp();
    textProp->displayName = "Text";
    textProp->value = comment->text;
    auto textWidget = new StringPropWidget();
    textWidget->setProp(textProp);
    textWidget->setMultiline(true);
    propWidgets.append(textWidget);

    connect(textWidget, &StringPropWidget::valueChanged, [=](const QString& value) {
        comment->text = value;
        emit commentPropertyChanged(comment);
    });
    layout->addWidget(textWidget);

    layout->addStretch(1);
}

void PropertiesWidget::clearSelection()
{
    displayMode = PropertyDisplayMode::None;
    selectedNode.clear();
    selectedFrame.clear();
    selectedComment.clear();

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