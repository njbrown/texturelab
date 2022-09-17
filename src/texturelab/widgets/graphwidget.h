#pragma once

#include <QMainWindow>

class NodeGraph;

class GraphWidget : public QMainWindow
{
public:
    GraphWidget();
    NodeGraph *graph;
};