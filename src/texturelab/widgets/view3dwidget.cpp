#include "view3dwidget.h"
#include "viewer3d.h"
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QString>
#include <QVector>

namespace {

// Available HDR environments.
//
// `rotation` is a yaw in degrees about the up axis, applied to both the skybox
// and the IBL lookups (Renderer::envRotation). Several of these HDRIs point
// their darkest quarter at the default camera, which sits on -Z, so the model
// opened as a silhouette. Each value was picked by maximising the diffuse
// irradiance on a normal 30 degrees off the camera-facing one, which lands the
// sky's brightest arc behind and to the left of the camera as a key light.
struct EnvInfo {
    QString displayName;
    QString resourcePath;
    float rotation;
};

const QVector<EnvInfo>& environments()
{
    static const QVector<EnvInfo> envList = {
        {"Docklands 01", ":env/docklands_01_1k.hdr", -100.0f},
        {"Golden Bay", ":env/golden_bay_1k.hdr", -85.0f},
        {"Little Paris Eiffel Tower", ":env/little_paris_eiffel_tower_1k.hdr",
         -95.0f},
        {"Sepulchral Chapel Basement", ":env/sepulchral_chapel_basement_1k.hdr",
         25.0f},
        {"St Peters Square Night", ":env/st_peters_square_night_1k.hdr",
         -95.0f},
        {"Stadium 01", ":env/stadium_01_1k.hdr", -120.0f},
        {"Studio Kontrast 03", ":env/studio_kontrast_03_1k.hdr", 100.0f},
        {"University Workshop", ":env/university_workshop_1k.hdr", 140.0f},
        {"Winter River", ":env/winter_river_1k.hdr", -100.0f}};
    return envList;
}

// The env the viewer opens on. Kept in sync with kFallbackEnvPath/Rotation in
// viewer3d.cpp, which covers hosts that never call setDefaultEnvironment().
const EnvInfo& defaultEnvironment()
{
    const QString defaultPath =
        QStringLiteral(":env/studio_kontrast_03_1k.hdr");
    for (const EnvInfo& env : environments()) {
        if (env.resourcePath == defaultPath)
            return env;
    }
    return environments().first();
}

} // namespace

View3DWidget::View3DWidget()
{
    this->viewer = new Viewer3D();
    this->setCentralWidget(viewer);

    const EnvInfo& defaultEnv = defaultEnvironment();
    this->viewer->setDefaultEnvironment(defaultEnv.resourcePath,
                                        defaultEnv.rotation);

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

    for (const EnvInfo& env : environments()) {
        QAction* envAction = envMenu->addAction(env.displayName);
        QString path = env.resourcePath;
        float rotation = env.rotation;
        connect(envAction, &QAction::triggered, [this, path, rotation]() {
            this->viewer->loadEnvironment(path, rotation);
        });
    }
}

void View3DWidget::reRender() { this->viewer->reRender(); }