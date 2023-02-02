#pragma once

#include <QMainWindow>
#include <QSharedPointer>

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
typedef QSharedPointer<TextureProject> TextureProjectPtr;
typedef QSharedPointer<TextureNode> TextureNodePtr;

class GraphWidget : public QMainWindow {
    Q_OBJECT

public:
    GraphWidget();

    void setTextureProject(TextureProjectPtr project);

    void dragEnterEvent(QDragEnterEvent* evt);
    void dragMoveEvent(QDragMoveEvent* event);
    void dropEvent(QDropEvent* event);

    void setTextureRenderer(TextureRenderer* renderer);

    nodegraph::NodeGraph* graph;
    // Library* library;
    nodegraph::ScenePtr scene;
    TextureProjectPtr project;

    TextureRenderer* renderer;

protected:
    void addNode(const TextureNodePtr& node);

signals:
    void nodeSelectionChanged(const TextureNodePtr& node);
    void nodeDoubleClicked(const TextureNodePtr& node);
};