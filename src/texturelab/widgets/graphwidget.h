#pragma once

#include <QMainWindow>

class NodeGraph;
class Library;

class TextureProject;
typedef QSharedPointer<TextureProject> TextureProjectPtr;

class GraphWidget : public QMainWindow
{
public:
    GraphWidget();

    void setTextureProject(TextureProjectPtr project);

    NodeGraph *graph;
    Library *library;
};