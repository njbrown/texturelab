#include "propwidgets.h"
#include "../../models.h"
#include "../../props.h"
#include "colorpicker.h"
#include "gradientpicker.h"

#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

const int SLIDER_MAX = 1000;

// FLOAT PROP WIDGET
// https://stackoverflow.com/a/19007951
FloatPropWidget::FloatPropWidget()
{
    prop = nullptr;

    auto vlayout = new QVBoxLayout(this);
    this->setLayout(vlayout);

    // label
    label = new QLabel(this);
    label->setText("");
    vlayout->addWidget(label);

    // slider
    slider = new QSlider(Qt::Horizontal, this);
    slider->setMinimum(0);
    slider->setMaximum(SLIDER_MAX);
    slider->setSingleStep(1);

    spinbox = new QDoubleSpinBox(this);

    auto hbox = new QHBoxLayout();
    hbox->addWidget(slider);
    hbox->addWidget(spinbox);

    vlayout->addLayout(hbox);

    this->setFixedHeight(80);

    connect(slider, &QSlider::valueChanged, [=](int val) {
        auto percent = val / (float)SLIDER_MAX;
        if (prop) {
            auto range = prop->maxValue - prop->minValue;
            auto finalValue = prop->minValue + range * percent;
            spinbox->setValue(finalValue);

            emit valueChanged(finalValue);
        }
    });

    connect(spinbox, &QDoubleSpinBox::valueChanged, [=](double val) {
        if (prop) {
            auto range = prop->maxValue - prop->minValue;
            auto finalValue = (val / range) * SLIDER_MAX;

            slider->setValue(finalValue);

            emit valueChanged(val);
        }
    });
}

void FloatPropWidget::setProp(FloatProp* prop)
{
    label->setText(prop->displayName);

    spinbox->setMinimum(prop->minValue);
    spinbox->setMaximum(prop->maxValue);
    spinbox->setSingleStep(prop->step);
    spinbox->setValue(prop->value);

    auto range = prop->maxValue - prop->minValue;
    auto finalValue = (prop->value / range) * SLIDER_MAX;

    slider->setValue(finalValue);

    this->prop = prop;
}

// INT PROP WIDGET
// https://stackoverflow.com/a/19007951
IntPropWidget::IntPropWidget()
{
    prop = nullptr;

    auto vlayout = new QVBoxLayout(this);
    this->setLayout(vlayout);

    // label
    label = new QLabel(this);
    label->setText("");
    vlayout->addWidget(label);

    // slider
    slider = new QSlider(Qt::Horizontal, this);
    slider->setMinimum(0);
    slider->setMaximum(SLIDER_MAX);
    slider->setSingleStep(1);

    spinbox = new QSpinBox(this);

    auto hbox = new QHBoxLayout();
    hbox->addWidget(slider);
    hbox->addWidget(spinbox);

    vlayout->addLayout(hbox);

    this->setFixedHeight(80);

    connect(slider, &QSlider::valueChanged, [=](int val) {
        auto percent = val / (float)SLIDER_MAX;
        if (prop) {
            spinbox->setValue(val);

            emit valueChanged(val);
        }
    });

    connect(spinbox, &QSpinBox::valueChanged, [=](int val) {
        if (prop) {
            slider->setValue(val);

            emit valueChanged(val);
        }
    });
}

void IntPropWidget::setProp(IntProp* prop)
{
    label->setText(prop->displayName);

    spinbox->setMinimum(prop->minValue);
    spinbox->setMaximum(prop->maxValue);
    spinbox->setSingleStep(prop->step);
    spinbox->setValue(prop->value);

    slider->setValue(prop->value);
    slider->setMinimum(prop->minValue);
    slider->setMaximum(prop->maxValue);
    slider->setSingleStep(prop->step);

    this->prop = prop;
}

// ENUM PROP WIDGET
// https://stackoverflow.com/a/19007951
EnumPropWidget::EnumPropWidget()
{
    prop = nullptr;

    auto vlayout = new QVBoxLayout(this);
    this->setLayout(vlayout);

    // label
    label = new QLabel(this);
    label->setText("");
    vlayout->addWidget(label);

    // slider
    comboBox = new QComboBox(this);
    vlayout->addWidget(comboBox);

    this->setFixedHeight(80);

    connect(comboBox, &QComboBox::currentIndexChanged,
            [=](int val) { emit valueChanged(val); });
}

void EnumPropWidget::setProp(EnumProp* prop)
{
    label->setText(prop->displayName);

    for (auto item : prop->values) {
        comboBox->addItem(item);
    }

    comboBox->setCurrentIndex(prop->index);

    this->prop = prop;
}

// BOOL PROP WIDGET
// https://stackoverflow.com/a/19007951
BoolPropWidget::BoolPropWidget()
{
    prop = nullptr;

    auto vlayout = new QVBoxLayout(this);
    this->setLayout(vlayout);

    // label
    label = new QLabel(this);
    label->setText("");
    vlayout->addWidget(label);

    // slider
    button = new QPushButton(this);
    vlayout->addWidget(button);

    this->setFixedHeight(80);

    connect(button, &QPushButton::pressed, [=]() {
        setValue(!this->value);
        emit valueChanged(this->value);
    });
}

void BoolPropWidget::setProp(BoolProp* prop)
{
    label->setText(prop->displayName);

    setValue(prop->value);
}

void BoolPropWidget::setValue(bool value)
{
    this->value = value;
    if (value) {
        button->setText("True");
    }
    else {
        button->setText("False");
    }
}

ColorPropWidget::ColorPropWidget()
{
    prop = nullptr;

    auto vlayout = new QVBoxLayout(this);
    this->setLayout(vlayout);

    // label
    label = new QLabel(this);
    label->setText("");
    vlayout->addWidget(label);

    // color preview
    colorPreview = new QWidget(this);
    colorPreview->setFixedHeight(20);
    colorPreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    colorPreview->setCursor(Qt::PointingHandCursor);
    colorPreview->installEventFilter(this);
    vlayout->addWidget(colorPreview);

    this->setFixedHeight(80);
}

void ColorPropWidget::setProp(ColorProp* prop)
{
    label->setText(prop->displayName);
    this->prop = prop;
    updateColorPreview();
}

void ColorPropWidget::updateColorPreview()
{
    if (prop) {
        QString styleSheet = QString("background-color: rgba(%1, %2, %3, %4); "
                                     "border: 1px solid #888;")
                                 .arg(prop->value.red())
                                 .arg(prop->value.green())
                                 .arg(prop->value.blue())
                                 .arg(prop->value.alpha());
        colorPreview->setStyleSheet(styleSheet);
    }
}

bool ColorPropWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == colorPreview && event->type() == QEvent::MouseButtonPress) {
        auto picker = new ColorPicker();
        picker->setColor(prop->value);

        // Position dialog below the colorPreview widget
        QPoint globalPos = colorPreview->mapToGlobal(QPoint(0, 0));
        picker->move(globalPos.x(), globalPos.y() + colorPreview->height());

        connect(picker, &ColorPicker::onColorChanged, this,
                [this](const QColor& color) {
                    if (prop) {
                        prop->value = color;
                        updateColorPreview();
                        emit valueChanged(color); // signal value changed
                    }
                });

        picker->exec();
        delete picker;

        return true;
    }
    return QWidget::eventFilter(obj, event);
}

// GRADIENT PROP WIDGET
GradientPropWidget::GradientPropWidget()
{
    prop = nullptr;

    auto vlayout = new QVBoxLayout(this);
    this->setLayout(vlayout);

    // label
    label = new QLabel(this);
    label->setText("");
    vlayout->addWidget(label);

    // gradient preview
    gradientPreview = new QWidget(this);
    gradientPreview->setFixedHeight(20);
    gradientPreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    gradientPreview->setCursor(Qt::PointingHandCursor);
    gradientPreview->installEventFilter(this);
    vlayout->addWidget(gradientPreview);

    this->setFixedHeight(80);
}

void GradientPropWidget::setProp(GradientProp* prop)
{
    label->setText(prop->displayName);
    this->prop = prop;
    updateGradientPreview();
}

void GradientPropWidget::updateGradientPreview()
{
    if (prop) {
        // Create a gradient preview using QLinearGradient
        QLinearGradient gradient(0, 0, 1, 0);
        gradient.setCoordinateMode(QGradient::StretchToDeviceMode);
        for (const auto& point : prop->value.points) {
            gradient.setColorAt(point.position, point.color);
        }

        QPalette palette;
        QPixmap pixmap(gradientPreview->width(), gradientPreview->height());
        QPainter painter(&pixmap);
        painter.fillRect(pixmap.rect(), gradient);
        painter.end();

        QString styleSheet = QString("border: 1px solid #888;");
        gradientPreview->setStyleSheet(styleSheet);

        // Set as background using palette
        palette.setBrush(gradientPreview->backgroundRole(), QBrush(pixmap));
        gradientPreview->setAutoFillBackground(true);
        gradientPreview->setPalette(palette);
    }
}

bool GradientPropWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == gradientPreview && event->type() == QEvent::MouseButtonPress) {
        auto picker = new GradientPickerDialog();
        picker->setGradient(prop->value);

        // Position dialog below the gradientPreview widget
        QPoint globalPos = gradientPreview->mapToGlobal(QPoint(0, 0));
        picker->move(globalPos.x(), globalPos.y() + gradientPreview->height());

        connect(picker, &GradientPickerDialog::onGradientAccepted, this,
                [this](const Gradient& gradient) {
                    if (prop) {
                        prop->value = gradient;
                        updateGradientPreview();
                        emit valueChanged(gradient); // signal value changed
                    }
                });

        picker->exec();
        delete picker;

        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void GradientPropWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateGradientPreview();
}

ImagePropWidget::ImagePropWidget()
{
    prop = nullptr;

    auto vlayout = new QVBoxLayout(this);
    this->setLayout(vlayout);

    // label
    label = new QLabel(this);
    label->setText("");
    vlayout->addWidget(label);

    // image preview
    imagePreview = new QLabel(this);
    imagePreview->setFixedHeight(100);
    imagePreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    imagePreview->setAlignment(Qt::AlignCenter);
    imagePreview->setCursor(Qt::PointingHandCursor);
    imagePreview->setStyleSheet(
        "QLabel { background-color: #333; border: 1px solid #888; }");
    imagePreview->setText("Click to select image");
    imagePreview->setScaledContents(false);
    imagePreview->installEventFilter(this);
    vlayout->addWidget(imagePreview);

    // clear button
    clearButton = new QPushButton("Clear Image", this);
    vlayout->addWidget(clearButton);

    connect(clearButton, &QPushButton::clicked, [this]() {
        if (prop) {
            prop->value = QImage();
            filePath.clear();
            updateImagePreview();
            emit valueChanged(QImage());
        }
    });

    this->setFixedHeight(180);
}

void ImagePropWidget::setProp(ImageProp* prop)
{
    label->setText(prop->displayName);
    this->prop = prop;
    updateImagePreview();
}

void ImagePropWidget::updateImagePreview()
{
    if (prop && !prop->value.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(prop->value);
        imagePreview->setPixmap(pixmap.scaled(imagePreview->size(),
                                              Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation));
    }
    else {
        imagePreview->setPixmap(QPixmap());
        imagePreview->setText("Click to select image");
    }
}

bool ImagePropWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == imagePreview && event->type() == QEvent::MouseButtonPress) {
        QString fileName = QFileDialog::getOpenFileName(
            this, "Select Image",
            filePath.isEmpty() ? QDir::homePath()
                               : QFileInfo(filePath).absolutePath(),
            "Image Files (*.png *.jpg *.jpeg *.bmp *.tiff *.tga *.webp);;All "
            "Files (*)");

        if (!fileName.isEmpty()) {
            filePath = fileName;
            QImage image(fileName);
            if (!image.isNull()) {
                if (prop) {
                    prop->value = image;
                    updateImagePreview();
                    emit valueChanged(image);
                }
            }
        }

        return true;
    }
    return QWidget::eventFilter(obj, event);
}