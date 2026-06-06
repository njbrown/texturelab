#pragma once

#include <QVector>
#include <QWidget>

class TextureProject;
class TextureNode;
class Comment;
class Frame;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureNode> TextureNodePtr;
typedef QSharedPointer<Comment> CommentPtr;
typedef QSharedPointer<Frame> FramePtr;

class EnumProp;
class IntProp;

enum class TextureChannel : int;

enum class PropertyDisplayMode { None, Node, Frame, Comment };

class PropertiesWidget : public QWidget {

    Q_OBJECT

    PropertyDisplayMode displayMode;
    QVector<QWidget*> propWidgets;

    TextureProjectPtr project;
    TextureNodePtr selectedNode;
    FramePtr selectedFrame;
    CommentPtr selectedComment;

    // base props
    EnumProp* textureChannelProp;
    IntProp* randomSeedProp;

public:
    PropertiesWidget();

    void setSelectedNode(const TextureNodePtr& node);
    void setSelectedFrame(const FramePtr& frame);
    void setSelectedComment(const CommentPtr& comment);
    void clearSelection();

    void setProject(const TextureProjectPtr& project);

private:
    void addBasePropsToLayout();

signals:
    void propertyUpdated(const QString& name, const QVariant& value);
    void textureChannelUpdated(const TextureChannel& name,
                               const TextureNodePtr& node);
    void framePropertyChanged(const FramePtr& frame);
    void commentPropertyChanged(const CommentPtr& comment);
};