#pragma once
#include "gradient.h"
#include <QDialog>
#include <QWidget>

// class QWidget;

class GradientSlider : QWidget {
    Gradient gradient;

public:
    GradientSlider();
    void setGradient(const Gradient& gradient);
    // void setPointColor(int index, const QColor& color);
    // sets color of selected point
    void setPointColor(const QColor& color);

private:
    void initUI();

signals:
    void onGradientChanged(const Gradient& gradient);

    // only the color is really used
    void onActivePointChanged(int index, const QColor& color);
};

class GradientPickerDialog : QDialog {
    // Gradient gradient;
    GradientSlider* gradientSlider = nullptr;

public:
    GradientPickerDialog();
    void setGradient(const Gradient& gradient);

private:
    void initUI();
};