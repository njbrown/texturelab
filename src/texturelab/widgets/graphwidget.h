#pragma once

#include <QMainWindow>
#include <QSharedPointer>

class QDragEnterEvent;
class TextureRenderer;
class NodeSearchPopup;

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
    void keyPressEvent(QKeyEvent* event) override;

    void setTextureRenderer(TextureRenderer* renderer);

    nodegraph::NodeGraph* graph;
    // Library* library;
    nodegraph::ScenePtr scene;
    TextureProjectPtr project;

    TextureRenderer* renderer;

protected:
    void addNode(const TextureNodePtr& node);
    void addNodeFromSearch(const QString& nodeName, const QPoint& position);

private:
    NodeSearchPopup* searchPopup;
    QPoint lastMousePos;

signals:
    void nodeSelectionChanged(const TextureNodePtr& node);
    void nodeDoubleClicked(const TextureNodePtr& node);
};