#include "props.h"

#include <QString>

QString createGuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString PropType::toString(Value propType)
{
    switch (propType) {
    case Float:
        return "float";
    case Int:
        return "int";
    case Bool:
        return "bool";
    case Color:
        return "color";
    case Enum:
        return "enum";
    case String:
        return "string";
    case Gradient:
        return "gradient";
    case Image:
        return "image";
    }

    return "unknown";
}

PropType::Value PropType::fromString(QString propType)
{
    if (propType == "float")
        return Float;
    if (propType == "int")
        return Int;
    if (propType == "bool")
        return Bool;
    if (propType == "color")
        return Color;
    if (propType == "enum")
        return Enum;
    if (propType == "string")
        return String;
    if (propType == "gradient")
        return Gradient;
    if (propType == "image")
        return Image;

    return Unknown;
}

Prop::Prop() { id = createGuid(); }

QJsonObject Prop::toJson()
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["displayName"] = displayName;
}
void Prop::fromJson(const QJsonObject& obj)
{
    id = obj["id"].toString();
    name = obj["name"].toString();
    displayName = obj["displayName"].toString();
}