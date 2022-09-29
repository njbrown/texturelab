#include <QApplication>
#include <QMainWindow>
#include <QStackedLayout>
#include "nodegraph.h"
#include "graph/scene.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QMainWindow w;

    auto graph = new NodeGraph(&w);

    // QStackedLayout *layout = new QStackedLayout(&w);
    // layout->addWidget(graph);

    w.setCentralWidget(graph);

    auto scene = graph->scene();

    NodePtr node(new Node());
    node->addInPort("image");
    node->addInPort("height");
    node->addOutPort("image");
    scene->addNode(node);

    NodePtr outputNode(new Node());
    outputNode->addInPort("image");
    outputNode->setPos(150, 150);
    scene->addNode(outputNode);

    scene->connectNodes(node, "image", outputNode, "image");

    NodePtr node2(new Node());
    node2->addInPort("image");
    node2->addOutPort("result");
    node2->setPos(250, 170);
    scene->addNode(node2);

    w.resize(800, 600);

    w.show();
    w.showMaximized();

    return a.exec();
}
