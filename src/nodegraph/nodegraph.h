#pragma once

#include <QtWidgets/QGraphicsView>
#include <QtCore/QRectF>
#include <QtCore/QPointF>
#include <QSharedPointer>

class QWidget;
class QWheelEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QWheelEvent;
class QShowEvent;
class Scene;
class Connection;
typedef QSharedPointer<Scene> ScenePtr;
typedef QSharedPointer<Connection> ConnectionPtr;
class Port;

struct MouseButtonStates
{
    bool left;
    bool middle;
    bool right;

    MouseButtonStates();

    // reset all to false
    void reset();
};

/*
This class draws a lot of inspiration from NodeGraphQt
https://github.com/jchanvfx/NodeGraphQt/blob/master/NodeGraphQt/widgets/viewer.py
*/
class NodeGraph
    : public QGraphicsView
{
public:
    NodeGraph(QWidget *parent = nullptr);

    ScenePtr scene() const { return _scene; }

    void scaleUp();

    void scaleDown();

protected:
    void setNodeGraphScene(ScenePtr scene);

    void wheelEvent(QWheelEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void keyReleaseEvent(QKeyEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void drawBackground(QPainter *painter, const QRectF &r) override;

    void showEvent(QShowEvent *event) override;

    bool eventFilter(QObject *o, QEvent *e) override;

    // events coming from the scene
    // void sceneKeyPressEvent(QKeyEvent *event);

    // void sceneKeyReleaseEvent(QKeyEvent *event);

    // NOTE: these functions return true if they swallow the event
    // this is because they're implemented using eventFilters and
    // that's how eventFilters work in qt
    bool sceneMousePressEvent(QGraphicsSceneMouseEvent *event);

    bool sceneMouseMoveEvent(QGraphicsSceneMouseEvent *event);

    bool sceneMouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    const Port *getPortAtScenePos(float x, float y) const;

private:
    QPointF _clickPos;
    ScenePtr _scene;
    MouseButtonStates mbStates;
    ConnectionPtr activeCon;
};