#include "project.h"
#include "libraries/library.h"
#include "props.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

TextureProjectPtr Project::loadTexture(QString path)
{
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QJsonParseError error;
    auto json = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error) {
        // report error
        return TextureProjectPtr(nullptr);
    }

    TextureProjectPtr texture(new TextureProject());

    // qDebug() << json["libraryVersion"].toString();

    // create library from version
    // Library *lib = new LibraryV1();
    Library* lib = createLibraryV2();

    // scene objects
    auto sceneObj = json["scene"].toObject();
    auto sceneNodesObj = sceneObj["nodes"].toObject();

    // load nodes
    auto nodeArray = json["nodes"].toArray();
    for (auto item : nodeArray) {
        auto nodeDef = item.toObject();
        auto nodeName = nodeDef["typeName"].toString();
        auto node = lib->createNode(nodeName);
        node->exportName = nodeDef["exportName"].toString("");
        node->id = nodeDef["id"].toString();
        node->randomSeed = (long)nodeDef["randomSeed"].toDouble(0);

        // get position from scene
        // we're converging the scene and designer props into one
        auto sceneObj = sceneNodesObj[node->id].toObject();
        auto x = sceneObj["x"].toDouble();
        auto y = sceneObj["y"].toDouble();
        node->pos = QVector2D(x, y);

        // add props
        auto propObj = nodeDef["properties"].toObject();
        for (auto key : propObj.keys()) {
            auto prop = node->getProp(key);
            if (prop == nullptr)
                continue;

            auto jsonProp = propObj[key];
            prop->fromJsonValue(jsonProp);
            qDebug() << "Loaded prop" << key << "for node" << prop->getValue();
            // if (jsonProp.isObject())
            //     prop->fromJsonValue(jsonProp.toObject());
            // else
            //     prop->setValue(jsonProp.toVariant());
        }

        texture->nodes[node->id] = node;
    }

    // load connections
    auto conArray = json["connections"].toArray();
    for (auto item : conArray) {
        auto conObj = item.toObject();

        QString leftNodeId = conObj["leftNodeId"].toString();
        TextureNodePtr leftNode = texture->getNodeById(leftNodeId);

        QString rightNodeId = conObj["rightNodeId"].toString();
        TextureNodePtr rightNode = texture->getNodeById(rightNodeId);

        QString rightNodeInputId = conObj["rightNodeInput"].toString();
        texture->addConnection(leftNode, rightNode, rightNodeInputId);
    }

    // load comments
    auto commentArray = sceneObj["comments"].toArray();
    for (auto item : commentArray) {
        auto obj = item.toObject();
        CommentPtr comment(new Comment());
        auto commentId = obj["id"].toString();
        comment->id = commentId.isEmpty()
                          ? QUuid::createUuid().toString(QUuid::WithoutBraces)
                          : commentId;
        comment->text = obj["text"].toString();
        comment->pos = QVector2D(obj["x"].toDouble(), obj["y"].toDouble());
        texture->comments[comment->id] = comment;
    }

    // load frames
    auto frameArray = sceneObj["frames"].toArray();
    for (auto item : frameArray) {
        auto obj = item.toObject();
        FramePtr frame(new Frame());
        auto frameId = obj["id"].toString();
        frame->id = frameId.isEmpty()
                        ? QUuid::createUuid().toString(QUuid::WithoutBraces)
                        : frameId;
        frame->text = obj["title"].toString();
        frame->pos = QVector2D(obj["x"].toDouble(), obj["y"].toDouble());
        frame->size =
            QVector2D(obj["width"].toDouble(300), obj["height"].toDouble(200));
        texture->frames[frame->id] = frame;
    }

    if (json["export"].isObject()) {
        auto exportObj = json["export"].toObject();
        texture->exportFilePattern =
            exportObj["filePattern"].toString("${project}_${name}");
        texture->exportDestination = exportObj["destination"].toString("");
    }

    if (json["editor"].isObject()) {
        auto editorObject = json["editor"].toObject();
        auto channels = editorObject["textureChannels"].toObject();
        for (auto key : channels.keys()) {
            TextureChannel channel = TextureChannel::None;
            if (key == "albedo")
                channel = TextureChannel::Albedo;
            else if (key == "normal")
                channel = TextureChannel::Normal;
            else if (key == "metalness")
                channel = TextureChannel::Metalness;
            else if (key == "roughness")
                channel = TextureChannel::Roughness;
            else if (key == "height")
                channel = TextureChannel::Height;
            else if (key == "alpha")
                channel = TextureChannel::Alpha;

            if (channel != TextureChannel::None) {
                auto nodeId = channels[key].toString();
                texture->textureChannels[channel] = nodeId;
            }
        }
    }

    texture->library = lib;

    return texture;
}