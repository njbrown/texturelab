#pragma once

#include <QMap>
#include <QIcon>
#include <QString>

class LibraryEntry
{
public:
    QString name;
    QString displayName;
    QIcon icon;
};

class Library
{
public:
    QMap<QString, LibraryEntry> items;
};