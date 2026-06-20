#include "colorpicker.h"
#include "./widgets.h"
#include <QApplication>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

ColorPicker::ColorPicker()
{
    // Frameless tool window instead of Qt::Popup: Qt::Popup does an X11
    // keyboard/pointer grab to detect outside clicks, which also blocks
    // global WM shortcuts (e.g. PrintScreen) while it's open. Outside
    // clicks are instead detected manually via the app-wide event filter
    // below, which doesn't require any grab.
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint);
    qApp->installEventFilter(this);

    svBox = new SVBox();
    hueSlider = new HueSlider();
    // alphaSlider = new AlphaSlider();

    svBox->setColor(QColor(255, 150, 0, 255));
    originalColor = QColor(255, 150, 0, 255);

    // connect widget signals to emit color changes
    connect(svBox, &SVBox::onSVChanged, this, [this](float s, float v) {
        QColor currentColor = svBox->getColor();
        emit onColorChanged(currentColor);
    });
    connect(hueSlider, &HueSlider::onHueChanged, this, [this](float h) {
        QColor currentColor = svBox->getColor();
        currentColor.setHsvF(h, currentColor.saturationF(),
                             currentColor.valueF());
        svBox->setColor(currentColor);
        emit onColorChanged(currentColor);
    });

    // add layout
    auto vlayout = new QVBoxLayout(this);
    vlayout->addWidget(svBox);
    vlayout->addWidget(hueSlider);
    // vlayout->addWidget(alphaSlider);

    this->setLayout(vlayout);

    // this->setBaseSize(400, 500);
    this->resize(400, 300);
}

void ColorPicker::setColor(const QColor& color)
{
    originalColor = color;
    svBox->setColor(color);
    hueSlider->setColor(color);
    // alphaSlider->setColor(color);
}

void ColorPicker::cancel()
{
    // revert to the color the dialog was opened with
    svBox->setColor(originalColor);
    hueSlider->setColor(originalColor);
    emit onColorChanged(originalColor);
    reject();
}

void ColorPicker::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        cancel();
        return;
    }

    event->ignore();

    // QDialog::keyPressEvent(event);
}

void ColorPicker::hideEvent(QHideEvent* event)
{
    QDialog::hideEvent(event);
    emit onClosed();
}

void ColorPicker::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // Tool windows aren't always given keyboard focus by the window
    // manager on their own, unlike Qt::Popup; claim it explicitly so
    // Escape reaches us.
    raise();
    activateWindow();
}

bool ColorPicker::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto widget = qobject_cast<QWidget*>(watched);
        if (widget && widget != this && !this->isAncestorOf(widget)) {
            close();
        }
    }

    return QDialog::eventFilter(watched, event);
}
