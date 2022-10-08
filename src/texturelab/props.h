#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QColor>

class Prop;

// http://techiesolves.blogspot.com/2018/01/base64-qstring-to-qimage-to-qstring-in.html
QString createGuid();
class PropType
{
public:
    enum Value
    {
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

class PropertyGroup
{
public:
    QString name;
    QList<Prop *> props;
};

class Prop
{
public:
    QString id;
    QString name;
    QString displayName;
    PropType::Value type;

    Prop();

    virtual QVariant getValue() = 0;
    virtual void setValue(QVariant val) = 0;

    virtual QJsonObject toJson();
    virtual void fromJson(const QJsonObject &obj);
};

class FloatProp : public Prop
{
public:
    double value;
    double minValue;
    double maxValue;
    double step;

    FloatProp()
    {
        value = 0;
        minValue = 0;
        maxValue = 1;
        step = 0.1;
        type = PropType::Float;
    }

    QVariant getValue()
    {
        return QVariant::fromValue(value);
    }

    void setValue(QVariant val)
    {
        value = val.toDouble();
    }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        obj["value"] = value;
        obj["minValue"] = minValue;
        obj["maxValue"] = maxValue;
        obj["step"] = step;
        return obj;
    }

    void fromJson(const QJsonObject &obj) override
    {
        value = obj["value"].toDouble();
        minValue = obj["minValue"].toDouble();
        maxValue = obj["maxValue"].toDouble();
        step = obj["step"].toDouble();
    }
};

class IntProp : public Prop
{
public:
    long value;
    long minValue;
    long maxValue;
    long step;

    IntProp()
    {
        value = 0;
        minValue = 0;
        maxValue = 100;
        step = 1;
        type = PropType::Int;
    }

    QVariant getValue()
    {
        return QVariant::fromValue(value);
    }

    void setValue(QVariant val)
    {
        value = (long)val.toLongLong();
    }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        obj["value"] = (qlonglong)value;
        obj["minValue"] = (qlonglong)minValue;
        obj["maxValue"] = (qlonglong)maxValue;
        obj["step"] = (qlonglong)step;
        return obj;
    }

    void fromJson(const QJsonObject &obj) override
    {
        value = obj["value"].toDouble();
        minValue = obj["minValue"].toDouble();
        maxValue = obj["maxValue"].toDouble();
        step = obj["step"].toDouble();
    }
};

class BoolProp : public Prop
{
public:
    bool value;

    BoolProp()
    {
        type = PropType::Bool;
        value = false;
    }

    QVariant getValue()
    {
        return value;
    }

    void setValue(QVariant val)
    {
        value = val.toBool();
    }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        obj["value"] = value ? "true" : "false";
        return obj;
    }

    void fromJson(const QJsonObject &obj) override
    {
        value = obj["value"].toBool();
    }
};

class EnumProp : public Prop
{
public:
    QList<QString> values;
    int index;

    EnumProp()
    {
        type = PropType::Color;
    }

    QVariant getValue()
    {
        return index;
    }

    void setValue(QVariant val)
    {
        index = val.value<int>();
    }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        obj["index"] = index;

        // values
        QJsonArray valueList;
        for (auto enumValue : values)
        {
            valueList.append(enumValue);
        }
        obj["values"] = valueList;

        return obj;
    }

    void fromJson(const QJsonObject &obj) override
    {
        index = obj["index"].toInt();

        auto list = obj["values"].toArray();
        values.empty();
        for (auto item : list)
        {
            values.append(item.toString());
        }
    }
};

struct ColorProp : public Prop
{
    QColor value;

    ColorProp()
    {
        type = PropType::Color;
    }

    QVariant getValue()
    {
        return value;
    }

    void setValue(QVariant val)
    {
        value = val.value<QColor>();
    }

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

    void fromJson(const QJsonObject &obj) override
    {
        auto colorObj = obj["value"].toObject();
        value.setRed(colorObj["r"].toInt());
        value.setGreen(colorObj["g"].toInt());
        value.setBlue(colorObj["b"].toInt());
        value.setAlpha(colorObj["a"].toInt());
    }
};

class StringProp : public Prop
{
public:
    QString value;

    StringProp()
    {
        type = PropType::Color;
    }

    QVariant getValue()
    {
        return value;
    }

    void setValue(QVariant val)
    {
        value = val.value<QString>();
    }

    QJsonObject toJson() override
    {
        auto obj = Prop::toJson();
        obj["value"] = value;
        return obj;
    }

    void fromJson(const QJsonObject &obj) override
    {
        value = obj["value"].toString();
    }
};