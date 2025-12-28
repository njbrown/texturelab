#pragma once
#include "gradient.h"
#include <QDialog>
#include <QGraphicsObject>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QPushButton>
#include <QWidget>

// class QWidget;

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
    qreal sliderY = 35;
    static constexpr qreal pointSize = 12;

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
    Q_OBJECT

    GradientSlider* gradientSlider = nullptr;
    QPushButton* colorButton = nullptr;

public:
    GradientPickerDialog();
    void setGradient(const Gradient& gradient);

private:
    void initUI();
    void onColorButtonClicked();
    void onControlPointSelected(int index, const QColor& color);
};