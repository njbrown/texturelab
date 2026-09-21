#include "curvepropwidget.h"

#include "thememanager.h"
#include "tokens.h"

#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QtMath>

// ============================================================================
// Metrics (colours are theme-driven; see CurveCanvas::refreshColors)
// ============================================================================

static constexpr int   CANVAS_PAD  = 8;  // px padding inside canvas
static constexpr float ANCHOR_R    = 5.0f;
static constexpr float ANCHOR_R_HL = 6.0f;
static constexpr float HANDLE_R    = 3.0f;

// ============================================================================
// CurveCanvas
// ============================================================================

CurveCanvas::CurveCanvas(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(200, 200);
    // Keep square
    QSizePolicy sp = sizePolicy();
    sp.setHeightForWidth(true);
    setSizePolicy(sp);

    refreshColors();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        refreshColors();
        update();
    });
}

void CurveCanvas::refreshColors()
{
    const Theme& t = ThemeManager::instance().theme();
    colBg         = t.color(Tokens::CurveBg);
    colGrid       = t.color(Tokens::CurveGrid);
    colIdentity   = t.color(Tokens::CurveIdentity);
    colCurve      = t.color(Tokens::CurveLine);
    colAnchorDef  = t.color(Tokens::CurveAnchor);
    colAnchorHov  = t.color(Tokens::CurveAnchorHover);
    colAnchorSel  = t.color(Tokens::CurveAnchorSelect);
    colHandleLine = t.color(Tokens::CurveHandleLine);
    colHandleDot  = t.color(Tokens::CurveHandleDot);
    colHandleHov  = t.color(Tokens::CurveHandleHover);
    colHandleCor  = t.color(Tokens::CurveHandleCorner);
}

int CurveCanvas::heightForWidth(int w) const { return w; }
bool CurveCanvas::hasHeightForWidth() const  { return true; }

void CurveCanvas::setCurve(const Curve& c)
{
    curve = c;
    update();
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

QPointF CurveCanvas::toWidget(float x, float y) const
{
    int pad = CANVAS_PAD;
    float W = width()  - 2 * pad;
    float H = height() - 2 * pad;
    // y is flipped: y=0 → bottom, y=1 → top
    return QPointF(pad + x * W, pad + (1.0f - y) * H);
}

QPointF CurveCanvas::toCurveSpace(QPointF p) const
{
    int pad = CANVAS_PAD;
    float W = width()  - 2 * pad;
    float H = height() - 2 * pad;
    float x = (p.x() - pad) / W;
    float y = 1.0f - (p.y() - pad) / H;
    return QPointF(qBound(0.0, x, 1.0), qBound(0.0, y, 1.0));
}

// ---------------------------------------------------------------------------
// Hit testing
// ---------------------------------------------------------------------------

int CurveCanvas::hitTestAnchor(QPointF pos, float radiusPx) const
{
    for (int i = 0; i < curve.points.size(); i++) {
        QPointF wp = toWidget(curve.points[i].x, curve.points[i].y);
        if (QLineF(pos, wp).length() <= radiusPx)
            return i;
    }
    return -1;
}

int CurveCanvas::hitTestHandle(QPointF pos, bool& outLeft, float radiusPx) const
{
    if (selectedPoint < 0 || selectedPoint >= curve.points.size())
        return -1;

    const CurvePoint& pt = curve.points[selectedPoint];

    QPointF lhW = toWidget(pt.x + pt.lx, pt.y + pt.ly);
    QPointF rhW = toWidget(pt.x + pt.rx, pt.y + pt.ry);

    if (QLineF(pos, lhW).length() <= radiusPx) { outLeft = true;  return selectedPoint; }
    if (QLineF(pos, rhW).length() <= radiusPx) { outLeft = false; return selectedPoint; }
    return -1;
}

int CurveCanvas::hitTestCurvePath(QPointF pos, float tolerancePx) const
{
    if (curve.points.size() < 2) return -1;

    // Sample the curve path and find closest point
    const int STEPS = 200;
    float minDist = tolerancePx;
    int   bestSeg = -1;
    float bestX   = 0.0f;

    for (int i = 0; i < curve.points.size() - 1; i++) {
        const CurvePoint& a = curve.points[i];
        const CurvePoint& b = curve.points[i + 1];

        for (int s = 0; s <= STEPS; s++) {
            float t   = s / (float)STEPS;
            float mt  = 1.0f - t;

            float px = mt*mt*mt*a.x + 3*mt*mt*t*(a.x+a.rx)
                     + 3*mt*t*t*(b.x+b.lx) + t*t*t*b.x;
            float py = mt*mt*mt*a.y + 3*mt*mt*t*(a.y+a.ry)
                     + 3*mt*t*t*(b.y+b.ly) + t*t*t*b.y;

            QPointF wp = toWidget(px, py);
            float d    = (float)QLineF(pos, wp).length();
            if (d < minDist) {
                minDist = d;
                bestSeg = i;
                bestX   = px;
            }
        }
    }
    (void)bestX;
    return bestSeg; // segment index — caller inserts a point at mouse x
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------

void CurveCanvas::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    drawGrid(p);
    drawIdentityLine(p);
    drawCurvePath(p);
    drawHandles(p);
    drawAnchors(p);
}

void CurveCanvas::drawGrid(QPainter& p)
{
    p.fillRect(rect(), colBg);

    QPen pen(colGrid, 1);
    p.setPen(pen);

    for (int i = 0; i <= 4; i++) {
        float t = i / 4.0f;
        QPointF a = toWidget(t, 0.0f);
        QPointF b = toWidget(t, 1.0f);
        p.drawLine(a, b);

        a = toWidget(0.0f, t);
        b = toWidget(1.0f, t);
        p.drawLine(a, b);
    }
}

void CurveCanvas::drawIdentityLine(QPainter& p)
{
    QPen pen(colIdentity, 1, Qt::DashLine);
    p.setPen(pen);
    p.drawLine(toWidget(0, 0), toWidget(1, 1));
}

void CurveCanvas::drawCurvePath(QPainter& p)
{
    if (curve.points.size() < 2) return;

    QPainterPath path;
    path.moveTo(toWidget(curve.points[0].x, curve.points[0].y));

    for (int i = 0; i < curve.points.size() - 1; i++) {
        const CurvePoint& a = curve.points[i];
        const CurvePoint& b = curve.points[i + 1];

        QPointF cp1 = toWidget(a.x + a.rx, a.y + a.ry);
        QPointF cp2 = toWidget(b.x + b.lx, b.y + b.ly);
        QPointF end = toWidget(b.x, b.y);
        path.cubicTo(cp1, cp2, end);
    }

    QPen pen(colCurve, 1.5f);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
}

void CurveCanvas::drawHandles(QPainter& p)
{
    if (selectedPoint < 0 || selectedPoint >= curve.points.size()) return;

    const CurvePoint& pt = curve.points[selectedPoint];
    QPointF anchor = toWidget(pt.x, pt.y);
    QPointF lh     = toWidget(pt.x + pt.lx, pt.y + pt.ly);
    QPointF rh     = toWidget(pt.x + pt.rx, pt.y + pt.ry);

    QColor dotColor = pt.smooth ? colHandleDot : colHandleCor;

    // Lines from anchor to handles
    QPen linePen(colHandleLine, 1);
    p.setPen(linePen);
    p.drawLine(anchor, lh);
    p.drawLine(anchor, rh);

    // Handle dots
    auto drawHandle = [&](QPointF pos, bool isHovered) {
        QColor c = isHovered ? colHandleHov : dotColor;
        p.setPen(QPen(c, 1));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(pos, HANDLE_R, HANDLE_R);
    };

    bool lhHovered = (hoveredHandle == selectedPoint && hoveredHandleLeft);
    bool rhHovered = (hoveredHandle == selectedPoint && !hoveredHandleLeft);
    drawHandle(lh, lhHovered);
    drawHandle(rh, rhHovered);
}

void CurveCanvas::drawAnchors(QPainter& p)
{
    for (int i = 0; i < curve.points.size(); i++) {
        const CurvePoint& pt = curve.points[i];
        QPointF wp = toWidget(pt.x, pt.y);

        float r;
        QColor fill;

        if (i == selectedPoint) {
            r = ANCHOR_R_HL;
            fill = colAnchorSel;
            // ring
            p.setPen(QPen(colAnchorSel, 1));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(wp, r + 2, r + 2);
        } else if (i == hoveredPoint) {
            r = ANCHOR_R_HL;
            fill = colAnchorHov;
        } else {
            r = ANCHOR_R;
            fill = colAnchorDef;
        }

        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawEllipse(wp, r, r);
    }
}

// ---------------------------------------------------------------------------
// Mouse events
// ---------------------------------------------------------------------------

void CurveCanvas::mousePressEvent(QMouseEvent* event)
{
    altHeld = event->modifiers() & Qt::AltModifier;

    if (event->button() == Qt::LeftButton) {
        QPointF pos = event->position();

        // 1. Hit test handle (only when a point is selected)
        bool handleLeft = false;
        int  hi = hitTestHandle(pos, handleLeft);
        if (hi >= 0) {
            dragTarget     = handleLeft ? CurveDragTarget::LeftHandle
                                        : CurveDragTarget::RightHandle;
            dragIndex      = hi;
            dragHandleLeft = handleLeft;
            setCursor(Qt::ClosedHandCursor);
            return;
        }

        // 2. Hit test anchor
        int ai = hitTestAnchor(pos);
        if (ai >= 0) {
            selectedPoint = ai;
            dragTarget    = CurveDragTarget::Anchor;
            dragIndex     = ai;
            setCursor(Qt::ClosedHandCursor);
            update();
            return;
        }

        // 3. Hit test curve path → add point on curve
        QPointF cs = toCurveSpace(pos);
        int pathSeg = hitTestCurvePath(pos);
        if (pathSeg >= 0) {
            // Snap y to existing curve at this x
            curve.addPoint((float)cs.x(), (float)cs.y());
            selectedPoint = -1;
            // Find the newly inserted point
            for (int i = 0; i < curve.points.size(); i++) {
                if (qAbs(curve.points[i].x - (float)cs.x()) < 0.01f) {
                    selectedPoint = i;
                    break;
                }
            }
            emit curveChanged(curve);
            update();
            return;
        }

        // 4. Empty area → add new point
        if (curve.points.size() < CURVE_MAX_POINTS) {
            curve.addPoint((float)cs.x(), (float)cs.y());
            selectedPoint = -1;
            for (int i = 0; i < curve.points.size(); i++) {
                if (qAbs(curve.points[i].x - (float)cs.x()) < 0.01f) {
                    selectedPoint = i;
                    break;
                }
            }
            emit curveChanged(curve);
            update();
        }
    } else if (event->button() == Qt::LeftButton) {
        // Deselect on background click (handled above via fall-through)
        selectedPoint = -1;
        update();
    }
}

void CurveCanvas::mouseMoveEvent(QMouseEvent* event)
{
    QPointF pos = event->position();
    altHeld = event->modifiers() & Qt::AltModifier;

    if (dragTarget == CurveDragTarget::Anchor && dragIndex >= 0) {
        QPointF cs = toCurveSpace(pos);
        curve.moveAnchor(dragIndex, (float)cs.x(), (float)cs.y());
        emit curveChanged(curve);
        emit anchorDragging(dragIndex);
        update();
        return;
    }

    if ((dragTarget == CurveDragTarget::LeftHandle ||
         dragTarget == CurveDragTarget::RightHandle) && dragIndex >= 0)
    {
        // Break symmetry if Alt held
        if (altHeld) curve.points[dragIndex].smooth = false;

        QPointF cs    = toCurveSpace(pos);
        const CurvePoint& pt = curve.points[dragIndex];
        float dx = (float)cs.x() - pt.x;
        float dy = (float)cs.y() - pt.y;

        bool isLeft = (dragTarget == CurveDragTarget::LeftHandle);
        // Compute delta from current handle position
        float curHx = isLeft ? pt.lx : pt.rx;
        float curHy = isLeft ? pt.ly : pt.ry;
        curve.moveHandle(dragIndex, isLeft, dx - curHx, dy - curHy);
        emit curveChanged(curve);
        update();
        return;
    }

    // Hover detection
    int prevHovAnchor = hoveredPoint;
    int prevHovHandle = hoveredHandle;
    hoveredPoint  = hitTestAnchor(pos);
    bool hl       = false;
    hoveredHandle = hitTestHandle(pos, hl);
    hoveredHandleLeft = hl;

    if (hoveredPoint >= 0 || hoveredHandle >= 0)
        setCursor(Qt::SizeAllCursor);
    else
        setCursor(Qt::CrossCursor);

    if (hoveredPoint != prevHovAnchor || hoveredHandle != prevHovHandle)
        update();
}

void CurveCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (dragTarget != CurveDragTarget::None)
            emit dragEnded();
        dragTarget = CurveDragTarget::None;
        dragIndex  = -1;
        setCursor(Qt::CrossCursor);
    }
}

void CurveCanvas::contextMenuEvent(QContextMenuEvent* event)
{
    int ai = hitTestAnchor(event->pos());
    if (ai < 0) return;

    QMenu menu(this);
    QAction* removeAct = menu.addAction("Remove Point");
    removeAct->setEnabled(ai > 0 && ai < curve.points.size() - 1);

    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == removeAct) {
        curve.removePoint(ai);
        if (selectedPoint == ai) selectedPoint = -1;
        else if (selectedPoint > ai) selectedPoint--;
        emit curveChanged(curve);
        update();
    }
}

void CurveCanvas::leaveEvent(QEvent*)
{
    hoveredPoint  = -1;
    hoveredHandle = -1;
    update();
}

// ============================================================================
// CurvePropWidget
// ============================================================================

CurvePropWidget::CurvePropWidget(CurveProp* prop, QWidget* parent)
    : QWidget(parent), prop(prop)
{
    auto* vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0, 0, 0, 4);
    vLayout->setSpacing(4);

    // Label row with Reset button
    auto* headerRow = new QHBoxLayout();
    headerRow->setContentsMargins(0, 0, 0, 0);

    auto* label = new QLabel(prop->displayName, this);
    resetBtn    = new QPushButton("Reset", this);
    resetBtn->setFixedWidth(50);
    resetBtn->setFixedHeight(20);
    resetBtn->setProperty("size", "small"); // styled in app.qss.in

    headerRow->addWidget(label);
    headerRow->addStretch();
    headerRow->addWidget(resetBtn);
    vLayout->addLayout(headerRow);

    // Canvas
    canvas = new CurveCanvas(this);
    canvas->setCurve(prop->value);
    vLayout->addWidget(canvas);

    // Readout label
    readout = new QLabel(this);
    readout->setObjectName("CurveReadout"); // styled in app.qss.in
    readout->setVisible(false);
    vLayout->addWidget(readout);

    setLayout(vLayout);

    // Connections
    connect(canvas, &CurveCanvas::curveChanged, this, [this](const Curve& c) {
        this->prop->value = c;
        emit valueChanged(c);
    });

    connect(canvas, &CurveCanvas::anchorDragging, this, [this](int idx) {
        if (idx >= 0 && idx < this->prop->value.points.size()) {
            const CurvePoint& pt = this->prop->value.points[idx];
            readout->setText(QString("In: %1   Out: %2")
                                 .arg(pt.x, 0, 'f', 2)
                                 .arg(pt.y, 0, 'f', 2));
            readout->setVisible(true);
        }
    });

    connect(canvas, &CurveCanvas::dragEnded, this, [this]() {
        readout->setVisible(false);
    });

    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        Curve identity;
        this->prop->value = identity;
        canvas->setCurve(identity);
        readout->setVisible(false);
        emit valueChanged(identity);
    });
}
