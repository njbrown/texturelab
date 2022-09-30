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
    node->setName("Warp");
    node->addInPort("image");
    node->addInPort("height");
    node->addOutPort("image");
    scene->addNode(node);

    NodePtr outputNode(new Node());
    outputNode->setName("Floodfill");
    outputNode->addInPort("image");
    outputNode->addOutPort("result");
    outputNode->setPos(200, 150);
    scene->addNode(outputNode);

    scene->connectNodes(node, "image", outputNode, "image");

    NodePtr node2(new Node());
    node2->setName("Stress Test");
    node2->addInPort("image");
    node2->addInPort("image2");
    node2->addInPort("image3");
    node2->addInPort("image4");
    node2->addOutPort("result");
    node2->setPos(320, 0);
    scene->addNode(node2);

    w.resize(800, 600);

    w.show();
    w.showMaximized();

    return a.exec();
}
