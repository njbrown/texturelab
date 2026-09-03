#include "clipboard.h"
#include "jsonutils.h"
#include "libraries/library.h"
#include "props.h"
#include <QApplication>
#include <QClipboard>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <limits>

const QString Clipboard::PREFIX = "texturelab-clipboard:";

void Clipboard::copyItems(TextureProjectPtr project,
                          const QList<QString>& nodeIds,
                          const QList<QString>& frameIds,
                          const QList<QString>& commentIds)
{
    QSet<QString> nodeIdSet(nodeIds.begin(), nodeIds.end());

    QJsonObject root;

    // Nodes
    QJsonArray nodeArray;
    for (const auto& id : nodeIds) {
        auto node = project->nodes.value(id);
        if (!node)
            continue;

        QJsonObject obj;
        obj["id"] = node->id;
        obj["typeName"] = node->typeName;
        obj["exportName"] = node->exportName;
        obj["randomSeed"] = (double)node->randomSeed;
        obj["x"] = node->pos.x();
        obj["y"] = node->pos.y();

        QJsonObject props;
        for (auto key : node->props.keys())
            props[key] = node->props[key]->toJsonValue();
        obj["properties"] = props;

        nodeArray.append(obj);
    }
    root["nodes"] = nodeArray;

    // Connections — only those fully within the selection
    QJsonArray conArray;
    for (auto& con : project->connections) {
        if (nodeIdSet.contains(con->leftNode->id) &&
            nodeIdSet.contains(con->rightNode->id)) {
            QJsonObject obj;
            obj["leftNodeId"] = con->leftNode->id;
            obj["rightNodeId"] = con->rightNode->id;
            obj["rightNodeInput"] = con->rightNodeInputName;
            conArray.append(obj);
        }
    }
    root["connections"] = conArray;

    // Comments
    QJsonArray commentArray;
    for (const auto& id : commentIds) {
        auto comment = project->comments.value(id);
        if (!comment)
            continue;
        QJsonObject obj;
        obj["text"] = comment->text;
        obj["x"] = comment->pos.x();
        obj["y"] = comment->pos.y();
        commentArray.append(obj);
    }
    root["comments"] = commentArray;

    // Frames
    QJsonArray frameArray;
    for (const auto& id : frameIds) {
        auto frame = project->frames.value(id);
        if (!frame)
            continue;
        QJsonObject obj;
        obj["title"] = frame->text;
        obj["color"] = frame->color.name(QColor::HexRgb);
        obj["x"] = frame->pos.x();
        obj["y"] = frame->pos.y();
        obj["width"] = frame->size.x();
        obj["height"] = frame->size.y();
        frameArray.append(obj);
    }
    root["frames"] = frameArray;

    QJsonDocument doc(root);
    QApplication::clipboard()->setText(PREFIX + doc.toJson(QJsonDocument::Compact));
}

bool Clipboard::hasData()
{
    return QApplication::clipboard()->text().startsWith(PREFIX);
}

bool Clipboard::pasteItems(TextureProjectPtr project,
                           QPointF viewCenter,
                           QList<TextureNodePtr>& outNodes,
                           QList<ConnectionPtr>& outConnections,
                           QList<CommentPtr>& outComments,
                           QList<FramePtr>& outFrames)
{
    if (!project || !project->library)
        return false;

    QString text = QApplication::clipboard()->text();
    if (!text.startsWith(PREFIX))
        return false;

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(text.mid(PREFIX.length()).toUtf8(), &err);
    if (err.error || !doc.isObject())
        return false;

    auto root = doc.object();

    // Compute bounding box of all items in the clipboard to find their center
    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();

    auto expandBBox = [&](double x, double y) {
        minX = std::min(minX, x); minY = std::min(minY, y);
        maxX = std::max(maxX, x); maxY = std::max(maxY, y);
    };

    for (auto item : root["nodes"].toArray()) {
        auto o = item.toObject();
        expandBBox(jsonutils::getDouble(o["x"]), jsonutils::getDouble(o["y"]));
    }
    for (auto item : root["comments"].toArray()) {
        auto o = item.toObject();
        expandBBox(jsonutils::getDouble(o["x"]), jsonutils::getDouble(o["y"]));
    }
    for (auto item : root["frames"].toArray()) {
        auto o = item.toObject();
        expandBBox(jsonutils::getDouble(o["x"]), jsonutils::getDouble(o["y"]));
        expandBBox(jsonutils::getDouble(o["x"]) + jsonutils::getDouble(o["width"]),
                   jsonutils::getDouble(o["y"]) + jsonutils::getDouble(o["height"]));
    }

    // If nothing in the bbox (empty clipboard somehow), fall back to no shift
    double offsetX = 0, offsetY = 0;
    if (minX <= maxX && minY <= maxY) {
        double bboxCenterX = (minX + maxX) / 2.0;
        double bboxCenterY = (minY + maxY) / 2.0;
        offsetX = viewCenter.x() - bboxCenterX;
        offsetY = viewCenter.y() - bboxCenterY;
    }

    // Build old→new node ID map
    QMap<QString, QString> nodeIdMap;
    for (auto item : root["nodes"].toArray()) {
        auto oldId = item.toObject()["id"].toString();
        nodeIdMap[oldId] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // Nodes
    for (auto item : root["nodes"].toArray()) {
        auto obj = item.toObject();
        auto node = project->library->createNode(obj["typeName"].toString());
        if (!node)
            continue;

        node->id = nodeIdMap[obj["id"].toString()];
        node->exportName = obj["exportName"].toString();
        node->randomSeed = jsonutils::getLong(obj["randomSeed"]);
        node->pos = QVector2D((float)(jsonutils::getDouble(obj["x"]) + offsetX),
                              (float)(jsonutils::getDouble(obj["y"]) + offsetY));

        auto propObj = obj["properties"].toObject();
        for (auto key : propObj.keys()) {
            auto prop = node->getProp(key);
            if (prop)
                prop->fromJsonValue(propObj[key]);
        }

        outNodes.append(node);
    }

    // Connections
    for (auto item : root["connections"].toArray()) {
        auto obj = item.toObject();
        auto newLeftId = nodeIdMap.value(obj["leftNodeId"].toString());
        auto newRightId = nodeIdMap.value(obj["rightNodeId"].toString());
        if (newLeftId.isEmpty() || newRightId.isEmpty())
            continue;

        TextureNodePtr leftNode, rightNode;
        for (auto& n : outNodes) {
            if (n->id == newLeftId)
                leftNode = n;
            if (n->id == newRightId)
                rightNode = n;
        }
        if (!leftNode || !rightNode)
            continue;

        auto con = ConnectionPtr(new Connection());
        con->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        con->leftNode = leftNode;
        con->rightNode = rightNode;
        con->leftNodeOutputName = "output";
        con->rightNodeInputName = obj["rightNodeInput"].toString();
        outConnections.append(con);
    }

    // Comments
    for (auto item : root["comments"].toArray()) {
        auto obj = item.toObject();
        auto comment = CommentPtr(new Comment());
        comment->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        comment->text = obj["text"].toString();
        comment->pos = QVector2D((float)(jsonutils::getDouble(obj["x"]) + offsetX),
                                 (float)(jsonutils::getDouble(obj["y"]) + offsetY));
        outComments.append(comment);
    }

    // Frames
    for (auto item : root["frames"].toArray()) {
        auto obj = item.toObject();
        auto frame = FramePtr(new Frame());
        frame->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        frame->text = obj["title"].toString();
        frame->color = QColor(obj["color"].toString());
        frame->pos = QVector2D((float)(jsonutils::getDouble(obj["x"]) + offsetX),
                               (float)(jsonutils::getDouble(obj["y"]) + offsetY));
        frame->size = QVector2D(jsonutils::getFloat(obj["width"], 300),
                                jsonutils::getFloat(obj["height"], 200));
        outFrames.append(frame);
    }

    return !outNodes.isEmpty() || !outComments.isEmpty() || !outFrames.isEmpty();
}
