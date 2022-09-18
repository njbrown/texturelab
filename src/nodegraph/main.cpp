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

    Scene *scene = new Scene();

    auto node = new Node();
    node->addInPort("image");
    node->addInPort("height");

    scene->addItem(node);
    graph->setScene(scene);

    w.resize(800, 600);

    w.show();
    w.showMaximized();

    return a.exec();
}
