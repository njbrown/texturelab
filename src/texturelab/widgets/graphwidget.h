#pragma once

#include <QMainWindow>
#include <QSharedPointer>

namespace nodegraph {
class NodeGraph;
class Scene;
typedef QSharedPointer<Scene> ScenePtr;

} // namespace nodegraph
class Library;

class TextureProject;
typedef QSharedPointer<TextureProject> TextureProjectPtr;

class GraphWidget : public QMainWindow {
public:
  GraphWidget();

  void setTextureProject(TextureProjectPtr project);

  nodegraph::NodeGraph *graph;
  Library *library;
  nodegraph::ScenePtr scene;
};