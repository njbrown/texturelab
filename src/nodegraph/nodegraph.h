#pragma once

#include <QtWidgets/QGraphicsView>
#include <QtCore/QRectF>
#include <QtCore/QPointF>

class QWidget;
class QWheelEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QWheelEvent;
class QShowEvent;

class NodeGraph
    : public QGraphicsView
{
public:
    NodeGraph(QWidget *parent = nullptr);

    void scaleUp();

    void scaleDown();

protected:
    void wheelEvent(QWheelEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void keyReleaseEvent(QKeyEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void drawBackground(QPainter *painter, const QRectF &r) override;

    void showEvent(QShowEvent *event) override;

private:
    QPointF _clickPos;
};