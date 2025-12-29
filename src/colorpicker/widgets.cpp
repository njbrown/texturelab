#include "widgets.h"
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

// https://github.com/yjg30737/pyqt-color-picker/blob/main/pyqt_color_picker/colorSquareWidget.py

SVBox::SVBox()
{
    // this->setFixedSize(400, 300);
    // this->setMinimumHeight(150);

    auto layout = new QGridLayout(this);
    colorWidget = new QWidget();

    blackWidget = new QWidget();
    // https://github.com/yjg30737/pyqt-color-picker/blob/main/pyqt_color_picker/style/black_overlay.css
    QString blackStyle = "background-color: qlineargradient(spread:pad, x1:0, "
                         "y1:0, x2:0, y2:1, stop:0 rgba(0, 0, 0, 0),"
                         "stop:1 rgba(0, 0, 0, 255));"
                         "width:100%;"
                         "border-radius: 5px;";
    blackWidget->setStyleSheet(blackStyle);

    blackWidget->installEventFilter(this);
    blackWidget->setMouseTracking(true);

    selector = new QWidget(blackWidget);
    selector->setGeometry(qFloor(selectorDiameter / 2) * -1,
                          qFloor(selectorDiameter / 2) * -1, selectorDiameter,
                          selectorDiameter);

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

void SVBox::setColor(const QColor& color)
{
    this->color = color;
    h = std::clamp(color.hueF(), 0.0f, 1.0f);
    s = color.saturationF();
    v = color.valueF();

    auto hue = h * 100;
    QString style = "background-color: qlineargradient(x1:1, x2:0, "
                    "stop:0 hsl(%1%,100%,50%),"
                    "stop:1 #fff);"
                    "border-radius: 5px;";
    auto formatted = style.arg(hue);

    // qDebug() << formatted;
    colorWidget->setStyleSheet(formatted);

    // Update selector position based on saturation and value
    int x = s * this->width() - selectorDiameter / 2;
    int y = (1.0f - v) * this->height() - selectorDiameter / 2;
    selector->move(x, y);

    this->selector->update();
    this->update();
    colorWidget->update();
}

QColor SVBox::getColor() const { return QColor::fromHsvF(h, s, v); }

bool SVBox::eventFilter(QObject* object, QEvent* event)
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

void SVBox::moveSelector(QMouseEvent* evt)
{
    auto pos = evt->pos();

    pos.setX(std::clamp(pos.x(), 0, this->width()));
    pos.setY(std::clamp(pos.y(), 0, this->height()));

    this->selector->move(pos -
                         QPoint(selectorDiameter / 2, selectorDiameter / 2));

    // calculate hsl
    s = (this->selector->pos().x() + (selectorDiameter / 2.0f)) / this->width();

    v = std::abs(((this->selector->pos().y() + (selectorDiameter / 2.0f)) /
                  this->height()) -
                 1.0f);

    // qDebug() << s << " " << v << "\n";
    emit onSVChanged(s, v);
}

// https://github.com/mortalis13/Qt-Color-Picker-Qt/blob/master/Widgets/ColorWidgets/hselector.cpp

HueSlider::HueSlider()
{
    selectorDrawn = false;
    hue = 0;
    // this->setFixedSize(400, 20);
    setFixedHeight(20);

    this->setStyleSheet("border-radius: 5px;");
}
void HueSlider::setHue(float hue)
{
    this->hue = hue;
    update();
}
void HueSlider::setColor(const QColor& color)
{
    this->color = color;
    this->hue = std::clamp(color.hueF(), 0.0f, 1.0f);
    update();
}

void HueSlider::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    float barHeight = height();
    float barWidth = width();

    if (!selectorDrawn) {
        // if (true) {
        selectorPixmap = QPixmap(barWidth, barHeight);
        QPainter huePainter(&selectorPixmap);

        QPointF p1(0, 0);
        QPointF p2(width(), 0);
        // QPointF p1(0, 0);
        // QPointF p2(100, 100);
        QLinearGradient grad(p1, p2);
        // QLinearGradient grad(p1, p2);

        float ratio = 1.0 / 360.0f;

        QColor gradientColor;
        for (qreal hs = 0; hs < 1.0; hs += ratio) {
            gradientColor.setHsvF(hs, 1.0f, 1.0f);
            grad.setColorAt(hs, gradientColor);
        }

        // grad.setColorAt(0.0, Qt::red);
        // grad.setColorAt(0.5, Qt::green);
        // grad.setColorAt(1.0, Qt::blue);

        huePainter.setPen(Qt::NoPen);
        huePainter.setBrush(QBrush(grad));
        huePainter.drawRect(0, 0, barWidth, barHeight);

        selectorDrawn = true;
    }

    // selectorPixmap.save("./selector.png");
    painter.drawPixmap(0, 0, selectorPixmap);

    // draw selector
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(Qt::white);
    const QPointF point(hue * width(), height() / 2);
    qDebug() << "Drawing selector at: " << point;
    painter.drawEllipse(point, 5, 5);
}

void HueSlider::resizeEvent(QResizeEvent* event) { selectorDrawn = false; }

void HueSlider::mousePressEvent(QMouseEvent* event)
{
    hue = event->pos().x() / (float)width();
    update();
    emit onHueChanged(hue);
}

void HueSlider::mouseMoveEvent(QMouseEvent* event)
{
    hue = event->pos().x() / (float)width();
    hue = std::clamp(hue, 0.0f, 1.0f);
    update();
    emit onHueChanged(hue);
}
