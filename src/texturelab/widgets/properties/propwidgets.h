#include <QVector>
#include <QWidget>

class QLabel;
class QSlider;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QPushButton;

class FloatProp;
class IntProp;
class EnumProp;
class BoolProp;
class ColorProp;
class ImageProp;

// https://stackoverflow.com/a/19007951
class FloatPropWidget : public QWidget {
    Q_OBJECT

    QLabel* label;
    QSlider* slider;
    QDoubleSpinBox* spinbox;

    FloatProp* prop;

public:
    FloatPropWidget();
    void setProp(FloatProp* prop);
signals:
    void valueChanged(float);
};

class IntPropWidget : public QWidget {
    Q_OBJECT

    QLabel* label;
    QSlider* slider;
    QSpinBox* spinbox;

    IntProp* prop;

public:
    IntPropWidget();
    void setProp(IntProp* prop);
signals:
    void valueChanged(long);
};

class EnumPropWidget : public QWidget {
    Q_OBJECT

    QLabel* label;
    QComboBox* comboBox;

    EnumProp* prop;

public:
    EnumPropWidget();
    void setProp(EnumProp* prop);
signals:
    void valueChanged(int);
};

class BoolPropWidget : public QWidget {
    Q_OBJECT

    QLabel* label;
    QPushButton* button;

    BoolProp* prop;
    bool value;

    void setValue(bool value);

public:
    BoolPropWidget();
    void setProp(BoolProp* prop);
signals:
    void valueChanged(bool);
};

class ColorPropWidget : public QWidget {
    Q_OBJECT

    QLabel* label;
    QWidget* colorPreview;

    ColorProp* prop;

    void updateColorPreview();

public:
    ColorPropWidget();
    void setProp(ColorProp* prop);
    bool eventFilter(QObject* obj, QEvent* event) override;
signals:
    void valueChanged(QColor);
};

class ImagePropWidget : public QWidget {
    Q_OBJECT

    QLabel* label;
    QLabel* imagePreview;
    QPushButton* clearButton;

    ImageProp* prop;
    QString filePath;

    void updateImagePreview();

public:
    ImagePropWidget();
    void setProp(ImageProp* prop);
    bool eventFilter(QObject* obj, QEvent* event) override;
signals:
    void valueChanged(QImage);
};
