#pragma once

#include <QUuid>

QString genGuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}