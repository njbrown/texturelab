#include "colorpicker.h"
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QWidget>

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
    SVBox()
    {
        this->setFixedSize(400, 300);

        auto layout = new QGridLayout(this);
        colorWidget = new QWidget();

        blackWidget = new QWidget();
        // https://github.com/yjg30737/pyqt-color-picker/blob/main/pyqt_color_picker/style/black_overlay.css
        QString blackStyle =
            "background-color: qlineargradient(spread:pad, x1:0, "
            "y1:0, x2:0, y2:1, stop:0 rgba(0, 0, 0, 0),"
            "stop:1 rgba(0, 0, 0, 255));"
            "width:100%;"
            "border-radius: 5px;";
        blackWidget->setStyleSheet(blackStyle);

        blackWidget->installEventFilter(this);
        blackWidget->setMouseTracking(true);

        selector = new QWidget(blackWidget);
        selector->setGeometry(qFloor(selectorDiameter / 2) * -1,
                              qFloor(selectorDiameter / 2) * -1,
                              selectorDiameter, selectorDiameter);

        selector->setStyleSheet("background-color: none;"
                                "border: 2px solid white;"
                                "border-radius: 5px;");

        // connect(blackWidget, &QWidget::mouseMoveEvent, []() {

        // });

        // you can stack widgets on top of each other in a grid

        layout->addWidget(colorWidget, 0, 0, 1, 1);
        layout->addWidget(blackWidget, 0, 0, 1, 1);
        layout->setContentsMargins(0, 0, 0, 0);
        this->setLayout(layout);

        this->setColor(QColor(255, 0, 0));
    }

    void setColor(const QColor& color)
    {
        h = color.hueF();
        s = color.saturationF();
        v = color.valueF();

        auto hue = color.hueF() * 100;
        QString style = "background-color: qlineargradient(x1:1, x2:0, "
                        "stop:0 hsl(%1%,100%,50%),"
                        "stop:1 #fff);"
                        "border-radius: 5px;";
        auto formatted = style.arg(hue);

        // qDebug() << formatted;
        this->setStyleSheet(formatted);
    }

    bool eventFilter(QObject* object, QEvent* event)
    {

        if (object == this->blackWidget) {
            auto mouseEvent = (QMouseEvent*)event;
            // handle appropriate event
            switch (event->type()) {
            case QEvent::MouseButtonPress: {
                if (mouseEvent->button() == Qt::LeftButton) {
                    dragging = true;
                    moveSelector(mouseEvent);
                }
            } break;
            case QEvent::MouseButtonRelease: {
                if (mouseEvent->button() == Qt::LeftButton) {
                    dragging = false;
                }
            } break;
            case QEvent::MouseMove: {
                if (dragging) {
                    //
                    moveSelector(mouseEvent);
                }
            } break;
            }
        }

        // false means it should be send to target also. as in , we dont remove
        // it. if you return true , you will take the event and widget never
        // sees it so be carefull with that.
        return false;
    }

    void moveSelector(QMouseEvent* evt)
    {
        auto pos = evt->pos();

        pos.setX(std::clamp(pos.x(), 0, this->width()));
        pos.setY(std::clamp(pos.y(), 0, this->height()));

        this->selector->move(
            pos - QPoint(selectorDiameter / 2, selectorDiameter / 2));

        // calculate hsl
        s = (this->selector->pos().x() + (selectorDiameter / 2.0f)) /
            this->width();

        v = std::abs(((this->selector->pos().y() + (selectorDiameter / 2.0f)) /
                      this->height()) -
                     1.0f);

        // qDebug() << s << " " << v << "\n";
    }

signals:
    void onSVChanged(float saturation, float value);
};

class HueSlider : public QWidget {
public:
    HueSlider() {}
    void setHue(float hue);
    void setColor(const QColor& color) {}
signals:
    void onHueChanged(float hue);
};

class AlphaSlider : public QWidget {
public:
    AlphaSlider() {}
    void setColor(const QColor& color) {}
};

ColorPicker::ColorPicker()
{
    svBox = new SVBox();
    hueSlider = new HueSlider();
    alphaSlider = new AlphaSlider();

    svBox->setColor(QColor(255, 150, 0, 255));

    // add layout
    auto vlayout = new QVBoxLayout(this);
    vlayout->addWidget(svBox);
    vlayout->addWidget(hueSlider);
    vlayout->addWidget(alphaSlider);

    this->setLayout(vlayout);
}

void ColorPicker::setColor(const QColor& color)
{
    svBox->setColor(color);
    // hueSlider->setColor(color);
    alphaSlider->setColor(color);
}