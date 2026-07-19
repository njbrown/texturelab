#include "project.h"
#include "libraries/library.h"
#include "libraries/libraryversionmigrator.h"
#include "libraries/libversion.h"
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
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error) {
        // report error
        return TextureProjectPtr(nullptr);
    }

    return Project::loadTextureFromJson(doc.object());
}

TextureProjectPtr Project::loadTextureFromJson(QJsonObject json)
{
    TextureProjectPtr texture(new TextureProject());

    // Pick the library matching this JSON's own version, so legacy
    // typeNames (e.g. "floodfill", "bevel", "perlin3d") that no longer
    // exist in the current library still resolve instead of crashing.
    // Callers that want the file upgraded should run it through
    // LibraryVersionMigrator first and pass in the migrated JSON.
    LibVersion version = LibraryVersionMigrator(json).sourceVersion();
    Library* lib = createLibraryForVersion(version);
    texture->libraryVersion = libVersionToString(version);

    // scene objects
    auto sceneObj = json["scene"].toObject();
    auto sceneNodesObj = sceneObj["nodes"].toObject();

    // load nodes
    auto nodeArray = json["nodes"].toArray();
    for (auto item : nodeArray) {
        auto nodeDef = item.toObject();
        auto nodeName = nodeDef["typeName"].toString();
        auto node = lib->createNode(nodeName);
        // createNode returns null for an unknown/legacy typeName the resolved
        // library can't build; skip it rather than dereferencing null.
        if (!node)
            continue;
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

        // Drop connections whose endpoints didn't load (unknown/legacy node
        // that was skipped, or a hand-edited/migrated file). Storing a
        // connection with a null endpoint would crash save/remove later.
        if (!leftNode || !rightNode)
            continue;

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
        auto colorStr = obj["color"].toString();
        if (!colorStr.isEmpty())
            frame->color = QColor(colorStr);
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
            else if (key == "ao")
                channel = TextureChannel::AO;

            if (channel != TextureChannel::None) {
                auto nodeId = channels[key].toString();
                texture->textureChannels[channel] = nodeId;
            }
        }
    }

    texture->library = lib;

    return texture;
}

QByteArray Project::saveTexture(TextureProjectPtr texture)
{
    QJsonObject json;

    // nodes
    QJsonArray nodeArray;
    for (auto& node : texture->nodes) {
        QJsonObject nodeDef;
        nodeDef["typeName"] = node->typeName;
        nodeDef["id"] = node->id;
        nodeDef["exportName"] = node->exportName;
        nodeDef["randomSeed"] = (double)node->randomSeed;

        QJsonObject propObj;
        for (auto key : node->props.keys()) {
            propObj[key] = node->props[key]->toJsonValue();
        }
        nodeDef["properties"] = propObj;

        nodeArray.append(nodeDef);
    }
    json["nodes"] = nodeArray;

    // scene (node positions, comments, frames)
    QJsonObject sceneObj;

    QJsonObject sceneNodesObj;
    for (auto& node : texture->nodes) {
        QJsonObject posObj;
        posObj["x"] = node->pos.x();
        posObj["y"] = node->pos.y();
        sceneNodesObj[node->id] = posObj;
    }
    sceneObj["nodes"] = sceneNodesObj;

    QJsonArray commentArray;
    for (auto& comment : texture->comments) {
        QJsonObject obj;
        obj["id"] = comment->id;
        obj["text"] = comment->text;
        obj["x"] = comment->pos.x();
        obj["y"] = comment->pos.y();
        commentArray.append(obj);
    }
    sceneObj["comments"] = commentArray;

    QJsonArray frameArray;
    for (auto& frame : texture->frames) {
        QJsonObject obj;
        obj["id"] = frame->id;
        obj["title"] = frame->text;
        obj["color"] = frame->color.name(QColor::HexRgb);
        obj["x"] = frame->pos.x();
        obj["y"] = frame->pos.y();
        obj["width"] = frame->size.x();
        obj["height"] = frame->size.y();
        frameArray.append(obj);
    }
    sceneObj["frames"] = frameArray;

    json["scene"] = sceneObj;

    // connections
    QJsonArray conArray;
    for (auto& con : texture->connections) {
        // Never serialize a connection with a missing endpoint (would crash on
        // the deref below and produce an unloadable file).
        if (!con || !con->leftNode || !con->rightNode)
            continue;
        QJsonObject conObj;
        conObj["leftNodeId"] = con->leftNode->id;
        conObj["rightNodeId"] = con->rightNode->id;
        conObj["rightNodeInput"] = con->rightNodeInputName;
        conArray.append(conObj);
    }
    json["connections"] = conArray;

    // library version this project's nodes/properties were saved against
    json["libraryVersion"] = texture->libraryVersion;

    // export settings
    QJsonObject exportObj;
    exportObj["filePattern"] = texture->exportFilePattern;
    exportObj["destination"] = texture->exportDestination;
    json["export"] = exportObj;

    // editor texture channels
    QJsonObject channelsObj;
    for (auto it = texture->textureChannels.begin();
         it != texture->textureChannels.end(); ++it) {
        QString key;
        switch (it.key()) {
        case TextureChannel::Albedo:    key = "albedo";    break;
        case TextureChannel::Normal:    key = "normal";    break;
        case TextureChannel::Metalness: key = "metalness"; break;
        case TextureChannel::Roughness: key = "roughness"; break;
        case TextureChannel::Height:    key = "height";    break;
        case TextureChannel::Alpha:     key = "alpha";     break;
        case TextureChannel::AO:        key = "ao";        break;
        default: continue;
        }
        channelsObj[key] = it.value();
    }
    QJsonObject editorObj;
    editorObj["textureChannels"] = channelsObj;
    json["editor"] = editorObj;

    return QJsonDocument(json).toJson();
}