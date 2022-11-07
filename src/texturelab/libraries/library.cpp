#include "library.h"
#include "../models.h"
#include "libv2.h"

#include <QMap>

TextureNodePtr Library::createNode(QString name)
{
    if (this->items.contains(name)) {
        auto& item = items[name];
        if (item.name == name) {
            auto node = item.factoryFunction();

            // todo: put this in the appropriate place
            node->init();

            return node;
        }
    }

    return TextureNodePtr(nullptr);
}

void Library::addNode(QString name, QString displayName, QString iconPath,
                      std::function<TextureNodePtr()> factoryFunction)
{

    LibraryEntry entry;
    entry.name = name;
    entry.displayName = displayName;
    entry.icon = QIcon(iconPath);
    entry.factoryFunction = factoryFunction;

    items[name] = entry;
}

bool Library::hasNode(QString name) { return items.contains(name); }

LibraryV1::LibraryV1() : Library()
{
    // add items
}

Library* createLibraryV2()
{
    auto lib = new Library();
    lib->addNode<PolygonNode>("polygon", "Polygon", ":nodes/bevel.png");
    lib->addNode<CircleNode>("circle", "Circle", ":nodes/bevel.png");
    lib->addNode<ColorNode>("color", "Color", ":nodes/color.png");
    lib->addNode<BlendNode>("blend", "Blend", ":nodes/blend.png");
    lib->addNode<OutputNode>("output", "Output", ":nodes/output.png");
    lib->addNode<NormalMapNode>("normalmap", "Normal Map", ":nodes/output.png");

    return lib;
}