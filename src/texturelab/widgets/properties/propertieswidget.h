#pragma once

#include <QHash>
#include <QSharedPointer>
#include <QUndoStack>
#include <QVariant>
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

namespace nodegraph { class Scene; }
typedef QSharedPointer<nodegraph::Scene> NgScenePtr;

class Prop;
class EnumProp;
class IntProp;
class TextureRenderer;

enum class TextureChannel : int;

enum class PropertyDisplayMode { None, Node, Frame, Comment };

class PropertiesWidget : public QWidget {

    Q_OBJECT

    PropertyDisplayMode displayMode;
    QVector<QWidget*> propWidgets;

    TextureProjectPtr project;
    NgScenePtr scene;
    QUndoStack* undoStack = nullptr;
    // non-owning; needed so undo/redo of a property change can kick the
    // render loop (the live edit path goes through propertyUpdated instead)
    TextureRenderer* renderer = nullptr;
    TextureNodePtr selectedNode;
    FramePtr selectedFrame;
    CommentPtr selectedComment;

    // base props
    EnumProp* textureChannelProp;
    IntProp* randomSeedProp;

    // Value each displayed prop held before the edit in progress. The color,
    // gradient, image and curve widgets write prop->value themselves before
    // emitting valueChanged, so the prop can't be read back for the undo
    // baseline — without this their undo steps record oldValue == newValue
    // and undoing them changes nothing.
    QHash<Prop*, QVariant> propBaselines;

public:
    PropertiesWidget();

    void setSelectedNode(const TextureNodePtr& node);
    void setSelectedFrame(const FramePtr& frame);
    void setSelectedComment(const CommentPtr& comment);
    void clearSelection();

    // Rebuilds the panel for the current selection, re-resolving it by id.
    // Called after undo/redo, which change values (and can replace the node
    // object) behind the widgets' backs — e.g. the curve editor keeps its own
    // copy of the curve and would re-apply an undone edit on the next touch.
    void refreshFromModel();

    void setProject(const TextureProjectPtr& project);
    void setScene(NgScenePtr ngScene);
    void setUndoStack(QUndoStack* stack);
    void setTextureRenderer(TextureRenderer* renderer);

    // Re-reads the undo baselines from the props. Call after undo/redo, which
    // changes prop values behind the panel's back.
    void syncPropBaselines();

private:
    void addBasePropsToLayout();
    // The prop's editor, wrapped with a variable link button when the prop
    // can be exposed.
    QWidget* createPropWidget(Prop* prop, const TextureNodePtr& node);
    QWidget* createPropEditor(Prop* prop, const TextureNodePtr& node);
    QWidget* wrapWithVariableLink(QWidget* editor, Prop* prop,
                                  const TextureNodePtr& node);

    // Variable actions from a prop's link menu. Each is one undo step.
    void exposeProp(const TextureNodePtr& node, Prop* prop);
    void linkProp(const TextureNodePtr& node, Prop* prop,
                  const QString& variableId);
    void unlinkProp(const TextureNodePtr& node, Prop* prop);

    // Returns the value the prop held before this edit and records newValue
    // as the baseline for the next one.
    QVariant takePropBaseline(Prop* prop, const QVariant& newValue);

signals:
    void propertyUpdated(const QString& name, const QVariant& value);
    void textureChannelUpdated(const TextureChannel& name,
                               const TextureNodePtr& node);
    void framePropertyChanged(const FramePtr& frame);
    void commentPropertyChanged(const CommentPtr& comment);
    // A prop was exposed/linked/unlinked: variables and prop values changed.
    void variablesChanged();
    // "Go to variable" / the link chip: show this variable.
    void variableActivated(const QString& variableId);
};