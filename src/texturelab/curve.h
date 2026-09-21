#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QVector>
#include <algorithm>

constexpr int CURVE_MAX_POINTS = 32;

struct CurvePoint {
    float x = 0.0f, y = 0.0f;
    float lx = -0.15f, ly = 0.0f; // left handle offset (lx <= 0)
    float rx =  0.15f, ry = 0.0f; // right handle offset (rx >= 0)
    bool  smooth = true;           // true: handles mirror through anchor
};

class Curve {
public:
    QVector<CurvePoint> points; // always sorted by x; minimum 2 points

    // Default: linear identity — (0,0) to (1,1) with horizontal handles
    Curve();

    // Insert sorted by x. Auto-computes handles tangent to the existing
    // curve at that x so the visible shape is preserved.
    void addPoint(float x, float y);

    // Remove by index. No-op for index 0 or last.
    void removePoint(int index);

    // Move anchor. Interior: x clamped between neighbours.
    // First/last: x locked at 0/1, only y moves.
    // If smooth, handles rotate to stay mirrored.
    void moveAnchor(int index, float x, float y);

    // Move one handle by a delta. If smooth=true, opposite handle mirrors.
    void moveHandle(int index, bool isLeft, float dx, float dy);

    QJsonObject toJson() const;
    static Curve fromJson(const QJsonObject& obj);

private:
    // Evaluate the derivative dy/dx at a given x using finite differences,
    // used for auto-tangent on addPoint.
    float derivAt(float x) const;

    // Cubic bezier helper: evaluate B(t) for one component
    static float cubicBez(float p0, float p1, float p2, float p3, float t);
    static float cubicBezD(float p0, float p1, float p2, float p3, float t);

    // Clamp handle offsets to maintain x-monotonicity constraints
    void clampHandles(int index);
};

Q_DECLARE_METATYPE(Curve)
