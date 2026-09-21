#pragma once

#include "../../curve.h"
#include "../../props.h"

#include <QColor>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

enum class CurveDragTarget { None, Anchor, LeftHandle, RightHandle };

class CurveCanvas : public QWidget {
    Q_OBJECT
public:
    explicit CurveCanvas(QWidget* parent = nullptr);

    void setCurve(const Curve& curve);
    const Curve& getCurve() const { return curve; }

    bool hasHeightForWidth() const override;
    int  heightForWidth(int w) const override;

signals:
    void curveChanged(const Curve& curve);
    void anchorDragging(int index); // emits index while dragging anchor
    void dragEnded();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    Curve curve;
    int selectedPoint  = -1;
    int hoveredPoint   = -1;
    int hoveredHandle  = -1;  // index of point whose handle is hovered
    bool hoveredHandleLeft = false;

    CurveDragTarget dragTarget = CurveDragTarget::None;
    int  dragIndex = -1;
    bool dragHandleLeft = false;
    QPointF dragStartCurve; // curve-space position at drag start
    bool altHeld = false;

    QPointF toWidget(float x, float y) const;
    QPointF toCurveSpace(QPointF widgetPos) const;

    int hitTestAnchor(QPointF pos, float radiusPx = 8.0f) const;
    // Returns point index; sets outLeft = true if left handle hit
    int hitTestHandle(QPointF pos, bool& outLeft, float radiusPx = 8.0f) const;
    // Returns point index to insert after if cursor is near the path
    int hitTestCurvePath(QPointF pos, float tolerancePx = 6.0f) const;

    void drawGrid(QPainter& p);
    void drawIdentityLine(QPainter& p);
    void drawCurvePath(QPainter& p);
    void drawAnchors(QPainter& p);
    void drawHandles(QPainter& p);

    // Theme colors, refreshed from the active theme (surface B: QSS can't reach
    // QPainter code). Repopulated on construction and on themeChanged().
    void refreshColors();
    QColor colBg, colGrid, colIdentity, colCurve;
    QColor colAnchorDef, colAnchorHov, colAnchorSel;
    QColor colHandleLine, colHandleDot, colHandleHov, colHandleCor;
};

class CurvePropWidget : public QWidget {
    Q_OBJECT
public:
    explicit CurvePropWidget(CurveProp* prop, QWidget* parent = nullptr);

signals:
    void valueChanged(const Curve& curve);

private:
    CurveProp*   prop;
    CurveCanvas* canvas;
    QLabel*      readout;
    QPushButton* resetBtn;
};
