#pragma once

#include <QMainWindow>

namespace nodegraph {
class NodeGraph;
}
class Library;

class TextureProject;
typedef QSharedPointer<TextureProject> TextureProjectPtr;

class GraphWidget : public QMainWindow {
public:
  GraphWidget();

  void setTextureProject(TextureProjectPtr project);

  nodegraph::NodeGraph *graph;
  Library *library;
};