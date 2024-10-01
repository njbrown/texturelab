#pragma once

#include <QColor>
#include <QVector>

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

    static Gradient defaultGradient();
};