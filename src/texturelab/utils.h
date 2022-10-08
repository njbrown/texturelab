#pragma once

#include <QWidget>

enum class Roles : int {
    ItemType = Qt::UserRole + 1,
    IdentityData = Qt::UserRole + 2,

    LibraryItemName = Qt::UserRole + 10
};

const QString LIBRARY_ITEM_MIME_FORMAT = "texturelab/library-item";