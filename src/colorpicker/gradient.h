#pragma once

#include <QColor>
#include <QMetaType>
#include <QVector>
#include <algorithm>

class GradientPoint {
public:
    float position;
    QColor color;

    GradientPoint() : position(0.0f), color(Qt::black) {}
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

    // Must match GRADIENT_MAX_POINTS in the node shaders
    // (texturelab/graphics/noderenderer.cpp): stops beyond it aren't rendered.
    static constexpr int MaxPoints = 32;

    // Returns false (and adds nothing) once the gradient is full.
    bool addPoint(GradientPoint point)
    {
        if (points.size() >= MaxPoints)
            return false;
        points.append(point);
        return true;
    }
    void sort();
    QColor sample(float position) const;

    static Gradient defaultGradient();
};

Q_DECLARE_METATYPE(Gradient)
Q_DECLARE_METATYPE(GradientPoint)