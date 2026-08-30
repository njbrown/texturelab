#include "view3dwidget.h"
#include "viewer3d.h"
#include <QAction>
#include <QMenu>
#include <QMenuBar>

View3DWidget::View3DWidget()
{
    this->viewer = new Viewer3D();
    this->setCentralWidget(viewer);

    this->viewer->setDefaultEnvironment(":env/studio_kontrast_03_1k.hdr");

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

    QAction* cylinderAction = modelMenu->addAction("Cylinder");
    connect(cylinderAction, &QAction::triggered,
            [this]() { this->viewer->setModel("cylinder"); });

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
        {"Docklands 01", ":env/docklands_01_1k.hdr"},
        {"Golden Bay", ":env/golden_bay_1k.hdr"},
        {"Little Paris Eiffel Tower",
         ":env/little_paris_eiffel_tower_1k.hdr"},
        {"Sepulchral Chapel Basement",
         ":env/sepulchral_chapel_basement_1k.hdr"},
        {"St Peters Square Night", ":env/st_peters_square_night_1k.hdr"},
        {"Stadium 01", ":env/stadium_01_1k.hdr"},
        {"Studio Kontrast 03", ":env/studio_kontrast_03_1k.hdr"},
        {"University Workshop", ":env/university_workshop_1k.hdr"},
        {"Winter River", ":env/winter_river_1k.hdr"}};

    for (const EnvInfo& env : envList) {
        QAction* envAction = envMenu->addAction(env.displayName);
        QString path = env.resourcePath;
        connect(envAction, &QAction::triggered,
                [this, path]() { this->viewer->loadEnvironment(path); });
    }
}

void View3DWidget::reRender() { this->viewer->reRender(); }