#pragma once

#include <QColor>
#include <QVector>
#include <algorithm>

class GradientPoint {
public:
    float position;
    QColor color;

    GradientPoint(float position, const QColor& color)
        : position(position), color(color)
    {
    }
};

class Gradient {
public:
    QVector<GradientPoint> points;

    Gradient() {}
    Gradient(const QVector<GradientPoint>& points) : points(points) {}
    Gradient(const Gradient& other) : points(other.points) {}

    void addPoint(GradientPoint point) { points.append(point); }
    void sort();
    QColor sample(float position) const;

    static Gradient defaultGradient();
};