#include "props.h"

#include <QString>

QString PropType::toString(Value propType)
{
    switch (propType)
    {
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