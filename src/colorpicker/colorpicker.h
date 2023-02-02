#pragma once
#include <QDialog>

class SVBox;
class HueSlider;
class AlphaSlider;

class ColorPicker : public QDialog {
public:
    ColorPicker();

    void setColor(const QColor& color);

signals:
    void onColorChanged(const QColor& color);
    void onClosed();

private:
    void initUI();
    void colorChangedByEditor(QColor color);
    void colorChangedByUI(QColor color);

    SVBox* svBox;
    HueSlider* hueSlider;
    AlphaSlider* alphaSlider;
};