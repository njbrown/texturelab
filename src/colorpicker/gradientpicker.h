#pragma once
#include "gradient.h"
#include <QDialog>
#include <QGraphicsObject>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QWidget>

class SVBox;
class HueSlider;

class GradientControlPoint : public QGraphicsObject {
    Q_OBJECT
public:
    int index;
    qreal size;
    QColor color;

    GradientControlPoint(int index, qreal x, qreal y, qreal size,
                         const QColor& color);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;
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
    qreal sliderY = 0;
    static constexpr qreal pointSize = 12;

    GradientSlider();
    void setGradient(const Gradient& gradient);
    Gradient getGradient() const { return gradient; }
    void setPointColor(const QColor& color);
    void selectFirstPoint();

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
    void resizeEvent(QResizeEvent* event) override;

signals:
    void onGradientChanged(const Gradient& gradient);
    void onActivePointChanged(int index, const QColor& color);
};

class GradientPickerDialog : public QDialog {
    Q_OBJECT

    GradientSlider* gradientSlider = nullptr;
    SVBox* svBox = nullptr;
    HueSlider* hueSlider = nullptr;
    bool updatingFromControlPoint = false;
    Gradient currentGradient;

public:
    GradientPickerDialog();
    void setGradient(const Gradient& gradient);

signals:
    void onGradientAccepted(const Gradient& gradient);

private:
    void initUI();
    void onControlPointSelected(int index, const QColor& color);
    void onSVChanged(float saturation, float value);
    void onHueChanged(float hue);
    void onAccepted();
    void onRejected();
};