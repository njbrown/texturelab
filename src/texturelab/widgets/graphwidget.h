#pragma once

#include <QMainWindow>

class NodeGraph;
class Library;

class GraphWidget : public QMainWindow
{
public:
    GraphWidget();

    NodeGraph *graph;
    Library *library;
};