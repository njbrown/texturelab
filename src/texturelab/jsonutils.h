#pragma once

#include <QJsonValue>
#include <QString>
#include <cmath>

// Older project files aren't strict about json types: numbers were sometimes
// written as strings ("1.5", "3"), ints as floats (3.0) and bools as
// "true"/"false". These helpers coerce whatever is in the json into the type
// the caller wants and fall back to defaultValue when the value is missing or
// can't be read as one.
namespace jsonutils {

inline double getDouble(const QJsonValue& val, double defaultValue = 0.0)
{
    if (val.isDouble())
        return val.toDouble();

    if (val.isString()) {
        bool ok = false;
        auto num = val.toString().trimmed().toDouble(&ok);
        return ok ? num : defaultValue;
    }

    if (val.isBool())
        return val.toBool() ? 1.0 : 0.0;

    return defaultValue;
}

inline float getFloat(const QJsonValue& val, float defaultValue = 0.0f)
{
    return (float)getDouble(val, (double)defaultValue);
}

inline long getLong(const QJsonValue& val, long defaultValue = 0)
{
    return (long)std::llround(getDouble(val, (double)defaultValue));
}

inline int getInt(const QJsonValue& val, int defaultValue = 0)
{
    return (int)getLong(val, (long)defaultValue);
}

inline bool getBool(const QJsonValue& val, bool defaultValue = false)
{
    if (val.isBool())
        return val.toBool();

    if (val.isDouble())
        return val.toDouble() != 0.0;

    if (val.isString()) {
        auto str = val.toString().trimmed().toLower();
        if (str == "true" || str == "yes")
            return true;
        if (str == "false" || str == "no" || str.isEmpty())
            return false;
        return getDouble(val, defaultValue ? 1.0 : 0.0) != 0.0;
    }

    return defaultValue;
}

inline QString getString(const QJsonValue& val,
                         const QString& defaultValue = QString())
{
    if (val.isString())
        return val.toString();

    if (val.isDouble()) {
        auto num = val.toDouble();
        return num == std::floor(num) ? QString::number((qlonglong)num)
                                      : QString::number(num);
    }

    if (val.isBool())
        return val.toBool() ? QStringLiteral("true") : QStringLiteral("false");

    return defaultValue;
}

} // namespace jsonutils
