#pragma once

#include "../colorpicker/gradient.h"
#include "curve.h"
#include "jsonutils.h"
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
        Image,
        Curve
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
    int order = 0; // for tracking order in UI

    PropertyGroup* group = nullptr;

    Prop();

    virtual QVariant getValue() = 0;
    virtual void setValue(QVariant val) = 0;
    virtual Prop* clone() const = 0;

    virtual QJsonObject toJson();
    virtual void fromJson(const QJsonObject& obj);

    // for old style direct value parsing
    virtual QJsonValue toJsonValue();
    virtual void fromJsonValue(const QJsonValue& obj);

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
        value = jsonutils::getDouble(obj["value"], value);
        minValue = jsonutils::getDouble(obj["minValue"], minValue);
        maxValue = jsonutils::getDouble(obj["maxValue"], maxValue);
        step = jsonutils::getDouble(obj["step"], step);
    }

    QJsonValue toJsonValue() override { return value; }

    void fromJsonValue(const QJsonValue& val) override
    {
        value = jsonutils::getDouble(val, value);
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
        value = jsonutils::getLong(obj["value"], value);
        minValue = jsonutils::getLong(obj["minValue"], minValue);
        maxValue = jsonutils::getLong(obj["maxValue"], maxValue);
        step = jsonutils::getLong(obj["step"], step);
    }

    QJsonValue toJsonValue() override { return (qlonglong)value; }

    void fromJsonValue(const QJsonValue& val) override
    {
        value = jsonutils::getLong(val, value);
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
        value = jsonutils::getBool(obj["value"], value);
    }

    QJsonValue toJsonValue() override { return value; }

    void fromJsonValue(const QJsonValue& val) override
    {
        value = jsonutils::getBool(val, value);
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
        index = jsonutils::getInt(obj["index"], index);

        auto list = obj["values"].toArray();
        values.clear();
        for (auto item : list) {
            values.append(item.toString());
        }
    }

    QJsonValue toJsonValue() override { return index; }

    void fromJsonValue(const QJsonValue& val) override
    {
        index = jsonutils::getInt(val, index);
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
        colObj["r"] = value.redF();
        colObj["g"] = value.greenF();
        colObj["b"] = value.blueF();
        colObj["a"] = value.alphaF();
        obj["value"] = colObj;

        return obj;
    }

    void fromJson(const QJsonObject& obj) override
    {
        Prop::fromJson(obj);
        auto colorObj = obj["value"].toObject();
        value.setRedF(jsonutils::getFloat(colorObj["r"]));
        value.setGreenF(jsonutils::getFloat(colorObj["g"]));
        value.setBlueF(jsonutils::getFloat(colorObj["b"]));
        value.setAlphaF(jsonutils::getFloat(colorObj["a"], 1.0f));
    }

    QJsonValue toJsonValue() override
    {
        QJsonObject colObj;
        colObj["r"] = value.redF();
        colObj["g"] = value.greenF();
        colObj["b"] = value.blueF();
        colObj["a"] = value.alphaF();
        return colObj;
    }

    void fromJsonValue(const QJsonValue& val) override
    {
        auto colorObj = val.toObject();
        value.setRedF(jsonutils::getFloat(colorObj["r"]));
        value.setGreenF(jsonutils::getFloat(colorObj["g"]));
        value.setBlueF(jsonutils::getFloat(colorObj["b"]));
        value.setAlphaF(jsonutils::getFloat(colorObj["a"], 1.0f));
    }
};

class StringProp : public Prop {
public:
    QString value;

    StringProp() : Prop() { type = PropType::String; }

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

    QJsonValue toJsonValue() override { return value; }

    void fromJsonValue(const QJsonValue& val) override
    {
        value = val.toString();
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
            float position = jsonutils::getFloat(pointObj["t"]);
            auto colorObj = pointObj["color"].toObject();
            QColor color;
            color.setRedF(jsonutils::getFloat(colorObj["r"]));
            color.setGreenF(jsonutils::getFloat(colorObj["g"]));
            color.setBlueF(jsonutils::getFloat(colorObj["b"]));
            color.setAlphaF(jsonutils::getFloat(colorObj["a"], 1.0f));

            value.addPoint(GradientPoint(position, color));
        }
    }

    QJsonValue toJsonValue() override
    {
        QJsonObject obj;
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

    void fromJsonValue(const QJsonValue& val) override
    {
        auto pointsArray = val.toObject()["points"].toArray();
        value.points.clear();

        for (const auto& pointValue : pointsArray) {
            auto pointObj = pointValue.toObject();
            float position = jsonutils::getFloat(pointObj["t"]);
            auto colorObj = pointObj["color"].toObject();
            QColor color;
            color.setRedF(jsonutils::getFloat(colorObj["r"]));
            color.setGreenF(jsonutils::getFloat(colorObj["g"]));
            color.setBlueF(jsonutils::getFloat(colorObj["b"]));
            color.setAlphaF(jsonutils::getFloat(colorObj["a"], 1.0f));

            value.addPoint(GradientPoint(position, color));
        }
    }
};

class CurveProp : public Prop {
public:
    Curve value; // default: linear identity

    CurveProp() : Prop() { type = PropType::Curve; }

    Prop* clone() const override
    {
        auto* copy = new CurveProp(*this);
        copy->group = nullptr;
        return copy;
    }

    QVariant getValue() override { return QVariant::fromValue(value); }

    void setValue(QVariant val) override { value = val.value<Curve>(); }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        obj["value"] = value.toJson();
        return obj;
    }

    void fromJson(const QJsonObject& obj) override
    {
        Prop::fromJson(obj);
        if (obj.contains("value") && obj["value"].isObject())
            value = Curve::fromJson(obj["value"].toObject());
        else
            value = Curve(); // fallback to linear identity
    }

    QJsonValue toJsonValue() override { return value.toJson(); }

    void fromJsonValue(const QJsonValue& val) override
    {
        if (val.isObject())
            value = Curve::fromJson(val.toObject());
        else
            value = Curve();
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
        if (parts.length() < 2)
            return;

        auto bytes = QByteArray::fromBase64(parts[1].toUtf8());

        QImage image;
        image.loadFromData(bytes);
        this->value = image;
        _textureDirty = true;
    }

    QJsonValue toJsonValue() override
    {
        if (value.isNull()) {
            return "";
        }

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        value.save(&buffer, "PNG");
        QString encoded = buffer.data().toBase64();

        return "data:image/png;base64," + encoded;
    }

    void fromJsonValue(const QJsonValue& val) override
    {
        auto stringData = val.toString();
        if (stringData.isNull() || stringData.isEmpty())
            return;

        auto parts = stringData.split(";base64,");
        if (parts.length() < 2)
            return;

        auto bytes = QByteArray::fromBase64(parts[1].toUtf8());

        QImage image;
        image.loadFromData(bytes);
        this->value = image;
        _textureDirty = true;
    }
};