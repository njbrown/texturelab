#pragma once

#include "../colorpicker/gradient.h"
#include <QBuffer>
#include <QColor>
#include <QIODevice>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QOpenGLTexture>
#include <QString>
#include <QUuid>
#include <atomic>

class Prop;
class PropertyGroup;
class QOpenGLTexture;

// http://techiesolves.blogspot.com/2018/01/base64-qstring-to-qimage-to-qstring-in.html
QString createGuid();
class PropType {
public:
    enum Value {
        Unknown = -1,
        Float = 0,
        Int,
        Bool,
        Color,
        Enum,
        String,
        Gradient,
        Image
    };

    static QString toString(Value propType);

    static Value fromString(QString propType);
};

class Prop {
public:
    QString id;
    QString name;
    QString displayName;
    PropType::Value type;

    PropertyGroup* group = nullptr;

    Prop();

    virtual QVariant getValue() = 0;
    virtual void setValue(QVariant val) = 0;
    virtual Prop* clone() const = 0;

    virtual QJsonObject toJson();
    virtual void fromJson(const QJsonObject& obj);

    virtual ~Prop() {}
};

class PropertyGroup {
public:
    QString name;
    QList<Prop*> props;
    bool collapsed = true;

    void add(Prop* prop)
    {
        this->props.append(prop);
        prop->group = this;
    }
};

class FloatProp : public Prop {
public:
    double value;
    double minValue;
    double maxValue;
    double step;

    FloatProp() : Prop()
    {
        value = 0;
        minValue = 0;
        maxValue = 1;
        step = 0.1;
        type = PropType::Float;
    }

    Prop* clone() const override
    {
        auto* copy = new FloatProp(*this);
        copy->group = nullptr;
        return copy;
    }

    QVariant getValue() override { return QVariant::fromValue(value); }

    void setValue(QVariant val) override { value = val.toDouble(); }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        obj["value"] = value;
        obj["minValue"] = minValue;
        obj["maxValue"] = maxValue;
        obj["step"] = step;
        return obj;
    }

    void fromJson(const QJsonObject& obj) override
    {
        Prop::fromJson(obj);
        value = obj["value"].toDouble();
        minValue = obj["minValue"].toDouble();
        maxValue = obj["maxValue"].toDouble();
        step = obj["step"].toDouble();
    }
};

class IntProp : public Prop {
public:
    long value;
    long minValue;
    long maxValue;
    long step;

    IntProp() : Prop()
    {
        value = 0;
        minValue = 0;
        maxValue = 100;
        step = 1;
        type = PropType::Int;
    }

    Prop* clone() const override
    {
        auto* copy = new IntProp(*this);
        copy->group = nullptr;
        return copy;
    }

    QVariant getValue() override { return QVariant::fromValue(value); }

    void setValue(QVariant val) override { value = (long)val.toDouble(); }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        obj["value"] = (qlonglong)value;
        obj["minValue"] = (qlonglong)minValue;
        obj["maxValue"] = (qlonglong)maxValue;
        obj["step"] = (qlonglong)step;
        return obj;
    }

    void fromJson(const QJsonObject& obj) override
    {
        Prop::fromJson(obj);
        value = obj["value"].toDouble();
        minValue = obj["minValue"].toDouble();
        maxValue = obj["maxValue"].toDouble();
        step = obj["step"].toDouble();
    }
};

class BoolProp : public Prop {
public:
    bool value;

    BoolProp() : Prop()
    {
        type = PropType::Bool;
        value = false;
    }

    Prop* clone() const override
    {
        auto* copy = new BoolProp(*this);
        copy->group = nullptr;
        return copy;
    }

    QVariant getValue() override { return value; }

    void setValue(QVariant val) override { value = val.toBool(); }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        obj["value"] = value ? "true" : "false";
        return obj;
    }

    void fromJson(const QJsonObject& obj) override
    {
        Prop::fromJson(obj);
        value = obj["value"].toBool();
    }
};

class EnumProp : public Prop {
public:
    QList<QString> values;
    int index;

    EnumProp() : Prop()
    {
        type = PropType::Enum;
        index = 0;
    }

    Prop* clone() const override
    {
        auto* copy = new EnumProp(*this);
        copy->group = nullptr;
        return copy;
    }

    QVariant getValue() override { return index; }

    void setValue(QVariant val) override { index = val.value<int>(); }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        obj["index"] = index;

        // values
        QJsonArray valueList;
        for (auto enumValue : values) {
            valueList.append(enumValue);
        }
        obj["values"] = valueList;

        return obj;
    }

    void fromJson(const QJsonObject& obj) override
    {
        Prop::fromJson(obj);
        index = obj["index"].toInt();

        auto list = obj["values"].toArray();
        values.empty();
        for (auto item : list) {
            values.append(item.toString());
        }
    }
};

struct ColorProp : public Prop {
    QColor value;

    ColorProp() : Prop() { type = PropType::Color; }

    Prop* clone() const override
    {
        auto* copy = new ColorProp(*this);
        copy->group = nullptr;
        return copy;
    }

    QVariant getValue() override { return value; }

    void setValue(QVariant val) override { value = val.value<QColor>(); }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        QJsonObject colObj;
        colObj["r"] = value.red();
        colObj["g"] = value.green();
        colObj["b"] = value.blue();
        colObj["a"] = value.alpha();
        obj["value"] = colObj;

        return obj;
    }

    void fromJson(const QJsonObject& obj) override
    {
        Prop::fromJson(obj);
        auto colorObj = obj["value"].toObject();
        value.setRed(colorObj["r"].toInt());
        value.setGreen(colorObj["g"].toInt());
        value.setBlue(colorObj["b"].toInt());
        value.setAlpha(colorObj["a"].toInt());
    }
};

class StringProp : public Prop {
public:
    QString value;

    StringProp() : Prop() { type = PropType::Color; }

    Prop* clone() const override
    {
        auto* copy = new StringProp(*this);
        copy->group = nullptr;
        return copy;
    }

    QVariant getValue() override { return value; }

    void setValue(QVariant val) override { value = val.value<QString>(); }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        obj["value"] = value;
        return obj;
    }

    void fromJson(const QJsonObject& obj) override
    {
        Prop::fromJson(obj);
        value = obj["value"].toString();
    }
};

class GradientProp : public Prop {
public:
    Gradient value;

    GradientProp() : Prop()
    {
        type = PropType::Gradient;
        value = Gradient::defaultGradient();
    }

    Prop* clone() const override
    {
        auto* copy = new GradientProp(*this);
        copy->group = nullptr;
        return copy;
    }

    QVariant getValue() override { return QVariant::fromValue(value); }

    void setValue(QVariant val) override { value = val.value<Gradient>(); }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();

        // Serialize gradient points
        QJsonArray pointsArray;
        for (const auto& point : value.points) {
            QJsonObject pointObj;
            pointObj["t"] = point.position;
            QJsonObject colorObj;
            colorObj["r"] = point.color.redF();
            colorObj["g"] = point.color.greenF();
            colorObj["b"] = point.color.blueF();
            colorObj["a"] = point.color.alphaF();
            pointObj["color"] = colorObj;
            pointsArray.append(pointObj);
        }
        obj["points"] = pointsArray;

        return obj;
    }

    void fromJson(const QJsonObject& obj) override
    {
        Prop::fromJson(obj);

        auto pointsArray = obj["points"].toArray();
        value.points.clear();

        for (const auto& pointValue : pointsArray) {
            auto pointObj = pointValue.toObject();
            float position = pointObj["t"].toDouble();
            auto colorObj = pointObj["color"].toObject();
            QColor color;
            color.setRedF(colorObj["r"].toDouble());
            color.setGreenF(colorObj["g"].toDouble());
            color.setBlueF(colorObj["b"].toDouble());
            color.setAlphaF(colorObj["a"].toDouble());

            value.addPoint(GradientPoint(position, color));
        }
    }
};

class ImageProp : public Prop {

public:
    QImage value;

    // when this is true, the next render should update the texture
    // with the image data
    // std::atomic<bool> _textureDirty = true;
    bool _textureDirty = true;
    // GLuint textureId = 0;
    QOpenGLTexture* texture = nullptr;

    ImageProp() : Prop() { type = PropType::Image; }

    Prop* clone() const override
    {
        auto* copy = new ImageProp(*this);
        copy->group = nullptr;
        return copy;
    }

    bool isTextureDirty() const { return _textureDirty; }
    void setTextureClean() { _textureDirty = false; }

    // Updates or creates the OpenGL texture from the image data
    // Should be called from the main OpenGL context thread
    void updateTexture();

    GLuint getTextureId() const;

    QVariant getValue() override { return value; }

    void setValue(QVariant val) override
    {
        value = val.value<QImage>();
        _textureDirty = true; // Mark texture as dirty when value changes
    }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        if (value.isNull()) {
            obj["value"] = "";
            return obj;
        }

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        value.save(&buffer, "PNG");
        QString encoded = buffer.data().toBase64();

        obj["value"] = "data:image/png;base64," + encoded;
        return obj;
    }

    void fromJson(const QJsonObject& obj) override
    {
        Prop::fromJson(obj);

        auto stringData = obj["value"].toString();
        if (stringData.isNull() || stringData.isEmpty())
            return;

        auto parts = stringData.split(";base64,");
        if (parts.length() == 0 || parts.length() == 1)
            return;

        auto bytes = QByteArray::fromBase64(parts[0].toUtf8());

        QImage image;
        image.loadFromData(QByteArray::fromBase64(stringData.toUtf8()));
        this->value = value;
    }
};