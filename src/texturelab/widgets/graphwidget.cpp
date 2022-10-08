#include "graphwidget.h"
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>

#include "./models.h"
#include "graph/scene.h"
#include "nodegraph.h"

GraphWidget::GraphWidget() : QMainWindow(nullptr) {
    graph = new nodegraph::NodeGraph(this);
    this->setCentralWidget(graph);

    library = nullptr;
}

void GraphWidget::setTextureProject(TextureProjectPtr project) {
    // generate nodes from texture project

    //   auto scene = graph->scene();
    // auto scene = nodegraph::Scene::create();
    auto scene = new nodegraph::Scene();

    // add nodes
    for (auto node : project->nodes) {
        auto gnode = nodegraph::Node::create();
        gnode->setName(node->title);
        for (auto input : node->inputs) {
            gnode->addInPort(input);
        }

        gnode->setId(node->id);
        gnode->addOutPort("output");
        gnode->setPos(node->pos.x(), node->pos.y());

        scene->addNode(gnode);
    }

    // add connections
    for (auto con : project->connections) {
        auto leftNode = scene->getNodeById(con->leftNode->id);
        auto rightNode = scene->getNodeById(con->rightNode->id);

        scene->connectNodes(leftNode, "output", rightNode,
                            con->rightNodeInputName);
    }

    // todo: add frames
    // todo: add comments
    // todo: add navigations

    graph->setNodeGraphScene(nodegraph::ScenePtr(scene));
}