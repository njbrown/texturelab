#include "project.h"
#include <QJsonDocument>
#include <QFile>

#include ""

TextureProjectPtr Project::loadTexture(QString path)
{
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QJsonParseError error;
    auto json = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error)
    {
        // report error
        return TextureProjectPtr(nullptr);
    }

    TextureProjectPtr texture(new TextureProject());

    qDebug() << json["libraryVersion"].toString();

    // create library from version
    Library *lib = new LibraryV1();

    // load nodes
    auto nodeArray = json["nodes"].toArray();
    for (auto nodeDef : nodeArray)
    {
    }

    return texture;
}