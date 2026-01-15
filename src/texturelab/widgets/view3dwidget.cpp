#include "view3dwidget.h"
#include "viewer3d.h"
#include <QAction>
#include <QMenu>
#include <QMenuBar>

View3DWidget::View3DWidget()
{
    this->viewer = new Viewer3D();
    this->setCentralWidget(viewer);

    this->viewer->setDefaultEnvironment(":env/cave_wall_1k.hdr");

    // Create menu bar
    QMenuBar* menuBar = new QMenuBar(this);
    this->setMenuBar(menuBar);

    // Model menu
    QMenu* modelMenu = menuBar->addMenu("Model");

    QAction* sphereAction = modelMenu->addAction("Sphere");
    connect(sphereAction, &QAction::triggered,
            [this]() { this->viewer->setModel("sphere"); });

    QAction* planeXYAction = modelMenu->addAction("Plane (XY)");
    connect(planeXYAction, &QAction::triggered,
            [this]() { this->viewer->setModel("plane_xy"); });

    QAction* planeYZAction = modelMenu->addAction("Plane (YZ)");
    connect(planeYZAction, &QAction::triggered,
            [this]() { this->viewer->setModel("plane_yz"); });

    QAction* planeXZAction = modelMenu->addAction("Plane (XZ)");
    connect(planeXZAction, &QAction::triggered,
            [this]() { this->viewer->setModel("plane_xz"); });

    QAction* cubeAction = modelMenu->addAction("Cube");
    connect(cubeAction, &QAction::triggered,
            [this]() { this->viewer->setModel("cube"); });

    QAction* cubesphereAction = modelMenu->addAction("CubeSphere");
    connect(cubesphereAction, &QAction::triggered,
            [this]() { this->viewer->setModel("cubesphere"); });

    // Environment menu
    QMenu* envMenu = menuBar->addMenu("Environment");

    // List of available HDR environments
    struct EnvInfo {
        QString displayName;
        QString resourcePath;
    };

    QVector<EnvInfo> envList = {
        {"Cave Wall", ":env/cave_wall_1k.hdr"},
        {"Christmas", ":env/christmas_1k.hdr"},
        {"Dresden Station Night", ":env/dresden_station_night_1k.hdr"},
        {"Hansaplatz", ":env/hansaplatz_1k.hdr"},
        {"Kloppenheim 05", ":env/kloppenheim_05_1k.hdr"},
        {"Modern Buildings Night", ":env/modern_buildings_night_1k.hdr"},
        {"Snowy Park 01", ":env/snowy_park_01_1k.hdr"},
        {"Spruit Sunrise", ":env/spruit_sunrise_1k.hdr"},
        {"Studio Small 07", ":env/studio_small_07_1k.hdr"},
        {"Wide Street 01", ":env/wide_street_01_1k.hdr"}};

    for (const EnvInfo& env : envList) {
        QAction* envAction = envMenu->addAction(env.displayName);
        QString path = env.resourcePath;
        connect(envAction, &QAction::triggered,
                [this, path]() { this->viewer->loadEnvironment(path); });
    }
}

void View3DWidget::reRender() { this->viewer->reRender(); }