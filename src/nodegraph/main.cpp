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
    scene->addItem(new Node());
    graph->setScene(scene);

    w.resize(800, 600);

    w.show();
    w.showMaximized();

    return a.exec();
}
