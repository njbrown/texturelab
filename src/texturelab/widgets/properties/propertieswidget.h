#pragma once

#include <QVector>
#include <QWidget>

class TextureProject;
class TextureNode;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureNode> TextureNodePtr;

class EnumProp;
class FloatProp;

enum class TextureChannel : int;

enum class PropertyDisplayMode { None, Node, Frame, Comment };

class PropertiesWidget : public QWidget {

    Q_OBJECT

    PropertyDisplayMode displayMode;
    QVector<QWidget*> propWidgets;

    TextureProjectPtr project;
    TextureNodePtr selectedNode;

    // base props
    EnumProp* textureChannelProp;
    FloatProp* randomSeedProp;

public:
    PropertiesWidget();

    void setSelectedNode(const TextureNodePtr& node);
    void clearSelection();

    void setProject(const TextureProjectPtr& project);

private:
    void addBasePropsToLayout();

signals:
    void propertyUpdated(const QString& name, const QVariant& value);
    void textureChannelUpdated(const TextureChannel& name,
                               const TextureNodePtr& node);
};