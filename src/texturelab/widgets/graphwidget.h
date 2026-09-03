#pragma once

#include "nodesearchpopup.h"
#include <QComboBox>
#include <QMainWindow>
#include <QSharedPointer>
#include <QSpinBox>
#include <QUndoStack>

class QAction;
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

    // Clipboard actions. These own the Cut/Copy/Paste shortcuts, scoped to
    // this widget so line edits elsewhere in the window keep their own, and
    // are reused by the main window's Edit menu so it shows the same keys
    // without registering a second, ambiguous binding.
    QAction* cutAction;
    QAction* copyAction;
    QAction* pasteAction;

protected:
    void addNode(const TextureNodePtr& node);
    void addItemFromSearch(const QString& name, PopupItemType type,
                           const QPoint& position);

private:
    void setupToolbar();

    // Breadcrumbs the change, and — when the driver tells us how much VRAM is
    // free — asks first if the new resolution plausibly won't fit. Returns
    // false if the user backed out.
    bool confirmResolutionChange(int from, int to);

    // Puts the picker back and explains, after TextureRenderer gave up on a
    // resolution and rolled the project back.
    void onResolutionChangeFailed(int requested, int fallback);

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