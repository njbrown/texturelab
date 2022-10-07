#include "graphwidget.h"
#include <QMainWindow>
#include <QToolBar>
#include <QMenu>
#include <QMenuBar>

#include "nodegraph.h"

GraphWidget::GraphWidget() : QMainWindow(nullptr)
{
    graph = new NodeGraph(this);
    this->setCentralWidget(graph);

    library = nullptr;
}