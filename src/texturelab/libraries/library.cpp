#include "../models.h"
#include "library.h"

#include <QMap>

TextureNodePtr Library::createNode(QString name)
{
    if (this->items.contains(name))
    {
        auto &item = items[name];
        if (item.name == name)
        {
            auto node = item.factoryFunction();
            return node;
        }
    }

    return TextureNodePtr(nullptr);
}

void Library::addNode(QString name,
                      QString displayName,
                      QString iconPath,
                      std::function<TextureNodePtr()> factoryFunction)
{

    LibraryEntry entry;
    entry.name = name;
    entry.displayName = displayName;
    entry.icon = QIcon(iconPath);
    entry.factoryFunction = factoryFunction;

    items[name] = entry;
}

bool Library::hasNode(QString name)
{
    return items.contains(name);
}

LibraryV1::LibraryV1() : Library()
{
    // add items
}