#pragma once

#include <QBuffer>
#include <QColor>
#include <QIODevice>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUuid>

class Prop;
class PropertyGroup;

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

    QVariant getValue() override { return QVariant::fromValue(value); }

    void setValue(QVariant val) override { value = (long)val.toLongLong(); }

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

class ImageProp : public Prop {
public:
    QImage value;

    ImageProp() : Prop() { type = PropType::Image; }

    QVariant getValue() override { return value; }

    void setValue(QVariant val) override { value = val.value<QImage>(); }

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