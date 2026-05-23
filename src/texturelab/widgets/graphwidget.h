#pragma once

#include "nodesearchpopup.h"
#include <QComboBox>
#include <QMainWindow>
#include <QSharedPointer>
#include <QSpinBox>

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
    void keyPressEvent(QKeyEvent* event) override;

    void setTextureRenderer(TextureRenderer* renderer);
    void syncPositionsToModel();

    nodegraph::NodeGraph* graph;
    // Library* library;
    nodegraph::ScenePtr scene;
    TextureProjectPtr project;

    TextureRenderer* renderer;

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

signals:
    void nodeSelectionChanged(const TextureNodePtr& node);
    void nodeDoubleClicked(const TextureNodePtr& node);
};