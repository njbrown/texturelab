#include "gradient.h"

void Gradient::sort()
{
    std::sort(points.begin(), points.end(),
              [](const GradientPoint& a, const GradientPoint& b) {
                  return a.position < b.position;
              });
}

QColor Gradient::sample(float position) const
{
    // Default color is black
    QColor color = Qt::black;

    if (points.size() == 0) {
        return color;
    }

    if (points.size() == 1) {
        return points[0].color;
    }

    // Find surrounding points
    int leftIdx = -1, rightIdx = -1;
    for (int i = 0; i < points.size(); ++i) {
        if (points[i].position <= position) {
            leftIdx = i;
        }
        if (points[i].position >= position && rightIdx == -1) {
            rightIdx = i;
            break;
        }
    }

    if (leftIdx >= 0 && rightIdx >= 0 && leftIdx != rightIdx) {
        // Interpolate between the two colors
        float t = (position - points[leftIdx].position) /
                  (points[rightIdx].position - points[leftIdx].position);
        QColor leftColor = points[leftIdx].color;
        QColor rightColor = points[rightIdx].color;

        color = QColor(
            leftColor.red() + t * (rightColor.red() - leftColor.red()),
            leftColor.green() + t * (rightColor.green() - leftColor.green()),
            leftColor.blue() + t * (rightColor.blue() - leftColor.blue()));
    }
    else if (leftIdx >= 0) {
        color = points[leftIdx].color;
    }
    else if (rightIdx >= 0) {
        color = points[rightIdx].color;
    }

    return color;
}

Gradient Gradient::defaultGradient()
{
    Gradient gradient;
    gradient.addPoint(GradientPoint(0.0, QColor(0, 0, 0, 255)));
    gradient.addPoint(GradientPoint(1.0, QColor(255, 255, 255, 255)));
    return gradient;
}