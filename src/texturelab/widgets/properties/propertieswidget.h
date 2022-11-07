#pragma once

#include <QVector>
#include <QWidget>

class TextureProject;
class TextureNode;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureNode> TextureNodePtr;

enum class PropertyDisplayMode { None, Node, Frame, Comment };

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