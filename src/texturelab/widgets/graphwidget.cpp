#include "graphwidget.h"
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>

#include "nodegraph.h"

GraphWidget::GraphWidget() : QMainWindow(nullptr) {
  graph = new nodegraph::NodeGraph(this);
  this->setCentralWidget(graph);

  library = nullptr;
}

void GraphWidget::setTextureProject(TextureProjectPtr project) {
  // generate nodes from texture project
}