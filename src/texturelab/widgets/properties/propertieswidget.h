#pragma once

#include <QVector>
#include <QWidget>

class TextureProject;
class TextureNode;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureNode> TextureNodePtr;

class QLabel;
class QSlider;
class QDoubleSpinBox;
class FloatProp;

enum class PropertyDisplayMode { None, Node, Frame, Comment };

// https://stackoverflow.com/a/19007951
class FloatPropWidget : public QWidget {
    Q_OBJECT

    QLabel* label;
    QSlider* slider;
    QDoubleSpinBox* spinbox;

    FloatProp* prop;

public:
    FloatPropWidget();
    void setProp(FloatProp* prop);
signals:
    void valueChanged(float);
};

class PropertiesWidget : public QWidget {

    Q_OBJECT

    PropertyDisplayMode displayMode;
    QVector<QWidget*> propWidgets;

    TextureProjectPtr project;

public:
    PropertiesWidget();

    void setSelectedNode(const TextureNodePtr& node);
    void clearSelection();

    void setProject(const TextureProjectPtr& project);

signals:
    void propertyUpdated(const QString& name, const QVariant& value);
};