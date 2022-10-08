#include "project.h"
#include "libraries/library.h"
#include <QJsonDocument>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>

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
    // Library *lib = new LibraryV1();
    Library *lib = createLibraryV2();

    // scene objects
    auto sceneObj = json["scene"].toObject();
    auto sceneNodesObj = sceneObj["nodes"].toObject();

    // load nodes
    auto nodeArray = json["nodes"].toArray();
    for (auto item : nodeArray)
    {
        auto nodeDef = item.toObject();

        auto node = lib->createNode(nodeDef["typeName"].toString());
        node->exportName = nodeDef["exportName"].toString("");
        node->id = nodeDef["id"].toString();
        node->randomSeed = nodeDef["randomSeed"].toInteger(0);

        // get position from scene
        // we're converging the scene and designer props into one
        auto sceneObj = sceneNodesObj[node->id].toObject();
        auto x = sceneObj["x"].toDouble();
        auto y = sceneObj["y"].toDouble();
        node->pos = QVector2D(x, y);

        // add props
        auto propObj = nodeDef["properties"].toObject();
        for (auto key : propObj.keys())
        {
            node->setProp(key, propObj[key].toVariant());
        }

        texture->nodes[node->id] = node;
    }

    // load connections
    auto conArray = json["connections"].toArray();
    for (auto item : conArray)
    {
        auto conObj = item.toObject();

        QString leftNodeId = conObj["leftNodeId"].toString();
        TextureNodePtr leftNode = texture->getNodeById(leftNodeId);

        QString rightNodeId = conObj["rightNodeId"].toString();
        TextureNodePtr rightNode = texture->getNodeById(rightNodeId);

        QString rightNodeInputId = conObj["rightNodeInput"].toString();
        texture->addConnection(leftNode, rightNode, rightNodeInputId);
    }

    texture->library = lib;

    return texture;
}