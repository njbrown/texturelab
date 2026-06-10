#include "propertieswidget.h"
#include "../../models.h"
#include "../../props.h"
#include "../../undo/undocommands.h"
#include "accordionwidget.h"
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

// Helper: push PropertyChangeCommand if undoStack is set; otherwise apply directly.
// The value is applied before calling this (first-redo pattern).
static void pushPropChange(QUndoStack* stack, TextureNodePtr node,
                            TextureProjectPtr project,
                            TextureRenderer* /*renderer*/,
                            const QString& propName,
                            QVariant oldVal, QVariant newVal)
{
    if (stack)
        stack->push(new PropertyChangeCommand(
            node, project, nullptr, propName, oldVal, newVal));
    // renderer=nullptr: PropertiesWidget doesn't hold the renderer;
    // markNodeAsDirty already triggers re-render via the renderer's update loop.
}

QWidget* PropertiesWidget::createPropWidget(Prop* prop,
                                            const TextureNodePtr& node)
{
    switch (prop->type) {
    case PropType::Float: {
        auto widget = new FloatPropWidget();
        widget->setProp((FloatProp*)prop);
        propWidgets.append(widget);
        connect(widget, &FloatPropWidget::valueChanged, [=](double value) {
            QVariant oldVal = prop->getValue();
            node->setProp(prop->name, value);
            project->markNodeAsDirty(node);
            emit propertyUpdated(prop->name, value);
            pushPropChange(undoStack, node, project, nullptr, prop->name, oldVal, value);
        });
        return widget;
    }
    case PropType::Bool: {
        auto widget = new BoolPropWidget();
        widget->setProp((BoolProp*)prop);
        propWidgets.append(widget);
        connect(widget, &BoolPropWidget::valueChanged, [=](bool value) {
            QVariant oldVal = prop->getValue();
            node->setProp(prop->name, value);
            project->markNodeAsDirty(node);
            emit propertyUpdated(prop->name, value);
            pushPropChange(undoStack, node, project, nullptr, prop->name, oldVal, value);
        });
        return widget;
    }
    case PropType::Int: {
        auto widget = new IntPropWidget();
        widget->setProp((IntProp*)prop);
        propWidgets.append(widget);
        connect(widget, &IntPropWidget::valueChanged, [=](long value) {
            QVariant oldVal = prop->getValue();
            node->setProp(prop->name, (int)value);
            project->markNodeAsDirty(node);
            emit propertyUpdated(prop->name, (int)value);
            pushPropChange(undoStack, node, project, nullptr, prop->name, oldVal, (int)value);
        });
        return widget;
    }
    case PropType::Enum: {
        auto widget = new EnumPropWidget();
        widget->setProp((EnumProp*)prop);
        propWidgets.append(widget);
        connect(widget, &EnumPropWidget::valueChanged, [=](int value) {
            QVariant oldVal = prop->getValue();
            node->setProp(prop->name, value);
            project->markNodeAsDirty(node);
            emit propertyUpdated(prop->name, value);
            pushPropChange(undoStack, node, project, nullptr, prop->name, oldVal, value);
        });
        return widget;
    }
    case PropType::Color: {
        auto widget = new ColorPropWidget();
        widget->setProp((ColorProp*)prop);
        propWidgets.append(widget);
        connect(widget, &ColorPropWidget::valueChanged, [=](const QColor& value) {
            QVariant oldVal = prop->getValue();
            node->setProp(prop->name, value);
            project->markNodeAsDirty(node);
            emit propertyUpdated(prop->name, value);
            pushPropChange(undoStack, node, project, nullptr, prop->name, oldVal, value);
        });
        return widget;
    }
    case PropType::Gradient: {
        auto widget = new GradientPropWidget();
        widget->setProp((GradientProp*)prop);
        propWidgets.append(widget);
        connect(widget, &GradientPropWidget::valueChanged, [=](const Gradient& value) {
            QVariant oldVal = prop->getValue();
            QVariant newVal = QVariant::fromValue(value);
            node->setProp(prop->name, newVal);
            project->markNodeAsDirty(node);
            emit propertyUpdated(prop->name, newVal);
            pushPropChange(undoStack, node, project, nullptr, prop->name, oldVal, newVal);
        });
        return widget;
    }
    case PropType::Image: {
        auto widget = new ImagePropWidget();
        widget->setProp((ImageProp*)prop);
        propWidgets.append(widget);
        connect(widget, &ImagePropWidget::valueChanged, [=](const QImage& value) {
            QVariant oldVal = prop->getValue();
            node->setProp(prop->name, value);
            project->markNodeAsDirty(node);
            emit propertyUpdated(prop->name, value);
            pushPropChange(undoStack, node, project, nullptr, prop->name, oldVal, value);
        });
        return widget;
    }
    case PropType::String: {
        auto widget = new StringPropWidget();
        widget->setProp((StringProp*)prop);
        propWidgets.append(widget);
        connect(widget, &StringPropWidget::valueChanged, [=](const QString& value) {
            QVariant oldVal = prop->getValue();
            node->setProp(prop->name, value);
            project->markNodeAsDirty(node);
            emit propertyUpdated(prop->name, value);
            pushPropChange(undoStack, node, project, nullptr, prop->name, oldVal, value);
        });
        return widget;
    }
    case PropType::Curve: {
        auto widget = new CurvePropWidget((CurveProp*)prop);
        propWidgets.append(widget);
        connect(widget, &CurvePropWidget::valueChanged, [=](const Curve& value) {
            QVariant oldVal = prop->getValue();
            QVariant newVal = QVariant::fromValue(value);
            node->setProp(prop->name, newVal);
            project->markNodeAsDirty(node);
            emit propertyUpdated(prop->name, newVal);
            pushPropChange(undoStack, node, project, nullptr, prop->name, oldVal, newVal);
        });
        return widget;
    }
    default:
        return nullptr;
    }
}

void PropertiesWidget::setSelectedNode(const TextureNodePtr& node)
{
    qDebug() << "Displaying properties for node: " << node->title;

    // clear current properties first, then assign (clearSelection resets selectedNode)
    this->clearSelection();
    this->selectedNode = node;

    auto layout = (QVBoxLayout*)this->layout();

    this->addBasePropsToLayout();

    // sort all props by insertion order
    QList<Prop*> sortedProps = node->props.values();
    std::sort(sortedProps.begin(), sortedProps.end(),
              [](Prop* a, Prop* b) { return a->order < b->order; });

    // ungrouped props first
    for (auto prop : sortedProps) {
        if (prop->group != nullptr)
            continue;
        auto widget = createPropWidget(prop, node);
        if (widget)
            layout->addWidget(widget);
    }

    // then each group as a collapsible accordion
    for (auto group : node->propertyGroups) {
        auto accordion =
            new AccordionWidget(group->name, group->collapsed, this);
        for (auto prop : group->props) {
            auto widget = createPropWidget(prop, node);
            if (widget)
                accordion->addWidget(widget);
        }
        layout->addWidget(accordion);
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
        long oldSeed = this->selectedNode->randomSeed;
        this->selectedNode->randomSeed = value;
        this->project->markNodeAsDirty(this->selectedNode);
        emit this->propertyUpdated("randomSeed", value);
        if (undoStack)
            undoStack->push(new RandomSeedChangeCommand(
                this->selectedNode, this->project, nullptr, oldSeed, value));
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
        if (undoStack) {
            QString oldTitle = frame->text;
            QColor  oldColor = frame->color;
            frame->text = value;
            emit framePropertyChanged(frame);
            undoStack->push(new EditFrameCommand(
                frame, scene, oldTitle, oldColor, value, oldColor));
        } else {
            frame->text = value;
            emit framePropertyChanged(frame);
        }
    });
    layout->addWidget(titleWidget);

    auto colorProp = new ColorProp();
    colorProp->displayName = "Color";
    colorProp->value = frame->color;
    auto colorWidget = new ColorPropWidget();
    colorWidget->setProp(colorProp);
    propWidgets.append(colorWidget);

    connect(colorWidget, &ColorPropWidget::valueChanged, [=](const QColor& color) {
        if (undoStack) {
            QString oldTitle = frame->text;
            QColor  oldColor = frame->color;
            frame->color = color;
            emit framePropertyChanged(frame);
            undoStack->push(new EditFrameCommand(
                frame, scene, oldTitle, oldColor, oldTitle, color));
        } else {
            frame->color = color;
            emit framePropertyChanged(frame);
        }
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
        if (undoStack) {
            QString oldText = comment->text;
            comment->text = value;
            emit commentPropertyChanged(comment);
            undoStack->push(new EditCommentCommand(comment, scene, oldText, value));
        } else {
            comment->text = value;
            emit commentPropertyChanged(comment);
        }
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

void PropertiesWidget::setScene(NgScenePtr ngScene)
{
    scene = ngScene;
}

void PropertiesWidget::setUndoStack(QUndoStack* stack)
{
    undoStack = stack;
}