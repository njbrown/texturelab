#pragma once

#include "models.h"
#include <QJsonObject>

class Project
{
public:
    static TextureProjectPtr loadTexture(QString path);

    // Builds a project from already-parsed (and possibly migrated) JSON.
    // Used directly by MainWindow when it has run the JSON through
    // LibraryVersionMigrator before constructing any node/library objects.
    static TextureProjectPtr loadTextureFromJson(QJsonObject json);

    static QByteArray saveTexture(TextureProjectPtr texture);
};