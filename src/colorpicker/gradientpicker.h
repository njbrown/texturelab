#pragma once
#include "gradient.h"
#include <QDialog>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QWidget>

// class QWidget;

class GradientControlPoint : public QGraphicsEllipseItem {
public:
    int index;

    GradientControlPoint(int index, qreal x, qreal y, qreal w, qreal h);
};

class GradientSlider : public QGraphicsView {
    Q_OBJECT

    Gradient gradient;
    QGraphicsScene* scene;
    QGraphicsRectItem* gradientRect;
    QVector<GradientControlPoint*> controlPoints;
    int selectedPointIndex = -1;
    int draggingPointIndex = -1;
    QPointF dragStartPos;

public:
    qreal sliderWidth = 380;
    qreal sliderHeight = 30;
    qreal sliderY = 35;

    GradientSlider();
    void setGradient(const Gradient& gradient);
    void setPointColor(const QColor& color);

private:
    void initUI();
    void updateGradientDisplay();
    void addControlPoint(float position, const QColor& color);
    void removeControlPoint(int index);
    void updateControlPointPositions();
    float positionFromX(qreal x);
    qreal xFromPosition(float position);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

signals:
    void onGradientChanged(const Gradient& gradient);
    void onActivePointChanged(int index, const QColor& color);
};

class GradientPickerDialog : public QDialog {
    // Gradient gradient;
    GradientSlider* gradientSlider = nullptr;

public:
    GradientPickerDialog();
    void setGradient(const Gradient& gradient);

private:
    void initUI();
};