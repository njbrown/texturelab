#include "gradient.h"

void Gradient::sort()
{
    std::sort(points.begin(), points.end(),
              [](const GradientPoint& a, const GradientPoint& b) {
                  return a.position < b.position;
              });
}

Gradient Gradient::defaultGradient()
{
    Gradient gradient;
    gradient.addPoint(GradientPoint(0.0, QColor(0, 0, 0, 255)));
    gradient.addPoint(GradientPoint(1.0, QColor(255, 255, 255, 255)));
    return gradient;
}