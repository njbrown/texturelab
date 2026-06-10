#pragma once

#include "nodesearchpopup.h"
#include <QComboBox>
#include <QMainWindow>
#include <QSharedPointer>
#include <QSpinBox>
#include <QUndoStack>

class QDragEnterEvent;
class TextureRenderer;

namespace nodegraph {
class NodeGraph;
class Scene;
typedef QSharedPointer<Scene> ScenePtr;

} // namespace nodegraph
class Library;

class TextureProject;
class TextureNode;
class Comment;
class Frame;
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureNode> TextureNodePtr;
typedef QSharedPointer<Comment> CommentPtr;
typedef QSharedPointer<Frame> FramePtr;

class GraphWidget : public QMainWindow {
    Q_OBJECT

public:
    GraphWidget();

    void setTextureProject(TextureProjectPtr project);
    void setUndoStack(QUndoStack* stack);

    void dragEnterEvent(QDragEnterEvent* evt);
    void dragMoveEvent(QDragMoveEvent* event);
    void dropEvent(QDropEvent* event);
    void keyPressEvent(QKeyEvent* event) override;

    void setTextureRenderer(TextureRenderer* renderer);
    void syncPositionsToModel();

    void syncFrameToScene(const FramePtr& frame);
    void syncCommentToScene(const CommentPtr& comment);

    nodegraph::NodeGraph* graph;
    // Library* library;
    nodegraph::ScenePtr scene;
    TextureProjectPtr project;

    TextureRenderer* renderer;
    QUndoStack* undoStack = nullptr;

protected:
    void addNode(const TextureNodePtr& node);
    void addItemFromSearch(const QString& name, PopupItemType type,
                           const QPoint& position);

private:
    void setupToolbar();

    NodeSearchPopup* searchPopup;
    QPoint lastMousePos;

    QComboBox* resolutionPicker;
    QSpinBox* seedInput;

public slots:
    void executeCopy();
    void executeCut();
    void executePaste();

signals:
    void nodeSelectionChanged(const TextureNodePtr& node);
    void nodeDoubleClicked(const TextureNodePtr& node);
    void frameSelectionChanged(const FramePtr& frame);
    void commentSelectionChanged(const CommentPtr& comment);
};