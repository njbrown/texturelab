#pragma once

#include <QMap>
#include <QIcon>
#include <QString>
#include <functional>
#include <QSharedPointer>

class TextureNode;
typedef QSharedPointer<TextureNode> TextureNodePtr;

class LibraryEntry
{
public:
    QString name;
    QString displayName;
    QIcon icon;
    std::function<TextureNodePtr()> factoryFunction;
};

class Library
{
public:
    QMap<QString, LibraryEntry> items;
    virtual TextureNodePtr createNode(QString name);

    void addNode(QString name, QString displayName, QString iconPath, std::function<TextureNodePtr()> factoryFunction);
    bool hasNode(QString name);

    template <typename T>
    void addNode(QString name,
                 QString displayName,
                 QString iconPath)
    {
        // https://stackoverflow.com/a/22934960
        static_assert(std::is_base_of<TextureNode, T>::value, "type parameter of this class must derive from TextureNode");

        this->addNode(name, displayName, iconPath, [=]()
                      { return TextureNodePtr(new T); });
    }
};

Library *createLibraryV2();

class LibraryV1 : public Library
{
public:
    LibraryV1();
};