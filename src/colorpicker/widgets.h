#pragma once

#include <QWidget>

class SVBox;
class HueSlider;
class AlphaSlider;

// https://github.com/yjg30737/pyqt-color-picker/blob/main/pyqt_color_picker/colorSquareWidget.py
class SVBox : public QWidget {
    QColor color;

    QWidget* colorWidget;
    // black overlay
    QWidget* blackWidget;

    QWidget* selector;
    int selectorDiameter = 10;
    bool dragging = false;

    float h, s, v;

public:
    SVBox();
    void setColor(const QColor& color);
    bool eventFilter(QObject* object, QEvent* event);
    void moveSelector(QMouseEvent* evt);

signals:
    void onSVChanged(float saturation, float value);
};

// https://github.com/mortalis13/Qt-Color-Picker-Qt/blob/master/Widgets/ColorWidgets/hselector.cpp
class HueSlider : public QWidget {
    float hue;
    bool selectorDrawn;
    QColor color;
    QPixmap selectorPixmap;

public:
    HueSlider();
    void setHue(float hue);
    void setColor(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
signals:
    void onHueChanged(float hue);
};
