#include "curve.h"

#include <QtMath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static float clamp01(float v) { return qBound(0.0f, v, 1.0f); }

static float snap(float v)
{
    return qRound(v * 1000.0f) / 1000.0f;
}

float Curve::cubicBez(float p0, float p1, float p2, float p3, float t)
{
    float mt = 1.0f - t;
    return mt*mt*mt*p0 + 3.0f*mt*mt*t*p1 + 3.0f*mt*t*t*p2 + t*t*t*p3;
}

float Curve::cubicBezD(float p0, float p1, float p2, float p3, float t)
{
    float mt = 1.0f - t;
    return 3.0f * (mt*mt*(p1-p0) + 2.0f*mt*t*(p2-p1) + t*t*(p3-p2));
}

// ---------------------------------------------------------------------------
// Curve
// ---------------------------------------------------------------------------

Curve::Curve()
{
    CurvePoint p0;
    p0.x = 0.0f; p0.y = 0.0f;
    p0.lx = -0.15f; p0.ly = 0.0f;
    p0.rx =  0.15f; p0.ry = 0.0f;
    p0.smooth = true;

    CurvePoint p1;
    p1.x = 1.0f; p1.y = 1.0f;
    p1.lx = -0.15f; p1.ly = 0.0f;
    p1.rx =  0.15f; p1.ry = 0.0f;
    p1.smooth = true;

    points.append(p0);
    points.append(p1);
}

float Curve::derivAt(float x) const
{
    // Find which segment contains x and compute dy/dx numerically
    const float eps = 0.001f;
    float x1 = qBound(0.0f, x - eps, 1.0f);
    float x2 = qBound(0.0f, x + eps, 1.0f);

    // Evaluate y at x1 and x2 by Newton-Raphson on each segment
    auto evalY = [&](float tx) -> float {
        if (points.size() < 2) return tx;
        int seg = points.size() - 2;
        for (int i = 0; i < points.size() - 1; i++) {
            if (tx <= points[i + 1].x) { seg = i; break; }
        }
        const CurvePoint& a = points[seg];
        const CurvePoint& b = points[seg + 1];
        float P0x = a.x,        P0y = a.y;
        float P1x = a.x + a.rx, P1y = a.y + a.ry;
        float P2x = b.x + b.lx, P2y = b.y + b.ly;
        float P3x = b.x,        P3y = b.y;

        float t = qBound(0.0f, (tx - P0x) / qMax(P3x - P0x, 1e-5f), 1.0f);
        for (int i = 0; i < 8; i++) {
            float bx  = cubicBez(P0x, P1x, P2x, P3x, t);
            float dbx = cubicBezD(P0x, P1x, P2x, P3x, t);
            if (qAbs(dbx) > 1e-6f) t -= (bx - tx) / dbx;
            t = qBound(0.0f, t, 1.0f);
        }
        return cubicBez(P0y, P1y, P2y, P3y, t);
    };

    float dy = evalY(x2) - evalY(x1);
    float dx = x2 - x1;
    return (dx > 1e-6f) ? dy / dx : 1.0f;
}

void Curve::addPoint(float x, float y)
{
    if (points.size() >= CURVE_MAX_POINTS) return;

    x = snap(clamp01(x));
    y = snap(clamp01(y));

    // Find insert position
    int insertPos = points.size();
    for (int i = 0; i < points.size(); i++) {
        if (points[i].x >= x) { insertPos = i; break; }
    }

    CurvePoint pt;
    pt.x = x;
    pt.y = y;
    pt.smooth = true;

    // Auto-tangent: set handles tangent to existing curve at this x
    float slope = derivAt(x);
    float handleLen = 0.1f;
    pt.rx =  handleLen;
    pt.ry =  slope * handleLen;
    pt.lx = -handleLen;
    pt.ly = -slope * handleLen;

    points.insert(insertPos, pt);
    clampHandles(insertPos);
}

void Curve::removePoint(int index)
{
    if (index <= 0 || index >= points.size() - 1) return;
    points.removeAt(index);
}

void Curve::clampHandles(int index)
{
    if (index < 0 || index >= points.size()) return;
    CurvePoint& pt = points[index];

    // rx must be >= 0
    pt.rx = qMax(pt.rx, 0.0f);
    // lx must be <= 0
    pt.lx = qMin(pt.lx, 0.0f);

    // Right handle absolute x must not exceed next anchor x
    if (index < points.size() - 1) {
        float maxRx = points[index + 1].x - pt.x;
        if (pt.rx > maxRx) {
            // Scale down proportionally
            float scale = (maxRx > 1e-6f) ? maxRx / pt.rx : 0.0f;
            pt.rx *= scale;
            pt.ry *= scale;
        }
    }

    // Left handle absolute x must not go below previous anchor x
    if (index > 0) {
        float minLx = points[index - 1].x - pt.x; // negative
        if (pt.lx < minLx) {
            float scale = (qAbs(minLx) > 1e-6f) ? minLx / pt.lx : 0.0f;
            pt.lx *= scale;
            pt.ly *= scale;
        }
    }
}

void Curve::moveAnchor(int index, float x, float y)
{
    if (index < 0 || index >= points.size()) return;
    CurvePoint& pt = points[index];

    y = snap(clamp01(y));

    if (index == 0) {
        // x locked
        pt.y = y;
    } else if (index == points.size() - 1) {
        // x locked
        pt.y = y;
    } else {
        float minX = points[index - 1].x + 0.005f;
        float maxX = points[index + 1].x - 0.005f;
        pt.x = snap(qBound(minX, x, maxX));
        pt.y = y;
    }

    clampHandles(index);
    if (index > 0) clampHandles(index - 1);
    if (index < points.size() - 1) clampHandles(index + 1);
}

void Curve::moveHandle(int index, bool isLeft, float dx, float dy)
{
    if (index < 0 || index >= points.size()) return;
    CurvePoint& pt = points[index];

    if (isLeft) {
        pt.lx = snap(pt.lx + dx);
        pt.ly = snap(pt.ly + dy);
        if (pt.smooth) {
            // Mirror to right handle (same length, opposite direction)
            float len = qSqrt(pt.lx*pt.lx + pt.ly*pt.ly);
            if (len > 1e-6f) {
                float rightLen = qSqrt(pt.rx*pt.rx + pt.ry*pt.ry);
                pt.rx =  (-pt.lx / len) * rightLen;
                pt.ry =  (-pt.ly / len) * rightLen;
            }
        }
    } else {
        pt.rx = snap(pt.rx + dx);
        pt.ry = snap(pt.ry + dy);
        if (pt.smooth) {
            float len = qSqrt(pt.rx*pt.rx + pt.ry*pt.ry);
            if (len > 1e-6f) {
                float leftLen = qSqrt(pt.lx*pt.lx + pt.ly*pt.ly);
                pt.lx =  (-pt.rx / len) * leftLen;
                pt.ly =  (-pt.ry / len) * leftLen;
            }
        }
    }

    clampHandles(index);
}

QJsonObject Curve::toJson() const
{
    QJsonArray arr;
    for (const auto& pt : points) {
        QJsonObject obj;
        obj["x"]      = (double)pt.x;
        obj["y"]      = (double)pt.y;
        obj["lx"]     = (double)pt.lx;
        obj["ly"]     = (double)pt.ly;
        obj["rx"]     = (double)pt.rx;
        obj["ry"]     = (double)pt.ry;
        obj["smooth"] = pt.smooth;
        arr.append(obj);
    }
    QJsonObject root;
    root["points"] = arr;
    return root;
}

Curve Curve::fromJson(const QJsonObject& obj)
{
    Curve curve;
    if (!obj.contains("points") || !obj["points"].isArray()) return curve;

    QJsonArray arr = obj["points"].toArray();
    if (arr.size() < 2) return curve;

    curve.points.clear();
    for (const auto& val : arr) {
        auto o = val.toObject();
        CurvePoint pt;
        pt.x  = clamp01((float)o["x"].toDouble());
        pt.y  = clamp01((float)o["y"].toDouble());
        pt.lx = qMin((float)o["lx"].toDouble(), 0.0f);
        pt.ly = (float)o["ly"].toDouble();
        pt.rx = qMax((float)o["rx"].toDouble(), 0.0f);
        pt.ry = (float)o["ry"].toDouble();
        pt.smooth = o["smooth"].toBool(true);
        curve.points.append(pt);
    }

    // Sort by x, enforce minimum 2 points
    std::sort(curve.points.begin(), curve.points.end(),
              [](const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });

    if (curve.points.size() < 2) return Curve(); // fallback to identity

    // Lock first/last x
    curve.points.first().x = 0.0f;
    curve.points.last().x  = 1.0f;

    return curve;
}
