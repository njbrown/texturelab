#pragma once

#include "models.h"
#include <QJsonObject>
#include <QStringList>

class Project
{
public:
    static TextureProjectPtr loadTexture(QString path);

    // Builds a project from already-parsed (and possibly migrated) JSON.
    // Used directly by MainWindow when it has run the JSON through
    // LibraryVersionMigrator before constructing any node/library objects.
    // Node typeNames the resolved library can't build are skipped; pass
    // skippedTypes to find out which, so callers can warn before a save
    // drops them for good.
    static TextureProjectPtr loadTextureFromJson(QJsonObject json,
                                                 QStringList* skippedTypes = nullptr);

    static QByteArray saveTexture(TextureProjectPtr texture);
};