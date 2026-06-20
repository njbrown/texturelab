#pragma once
#include <QDialog>

class SVBox;
class HueSlider;
class AlphaSlider;
class QKeyEvent;
class QHideEvent;
class QShowEvent;

class ColorPicker : public QDialog {
    Q_OBJECT
public:
    ColorPicker();

    void setColor(const QColor& color);

signals:
    void onColorChanged(const QColor& color);
    void onClosed();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void initUI();
    void colorChangedByEditor(QColor color);
    void colorChangedByUI(QColor color);
    void cancel();

    SVBox* svBox;
    HueSlider* hueSlider;
    AlphaSlider* alphaSlider;
    QColor originalColor;
};