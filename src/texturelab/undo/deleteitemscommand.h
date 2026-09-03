#pragma once

#include "../models.h"
#include <QColor>
#include <QJsonObject>
#include <QUndoCommand>
#include <QVector2D>

class TextureRenderer;

namespace nodegraph {
class Scene;
typedef QSharedPointer<Scene> ScenePtr;
} // namespace nodegraph

class DeleteItemsCommand : public QUndoCommand {
public:
    struct SerializedNode {
        QString typeName, id, exportName;
        long randomSeed;
        QVector2D pos;
        QJsonObject props;
    };

    struct SerializedConnection {
        QString id, leftNodeId, leftOutput, rightNodeId, rightInput;
    };

    struct SerializedFrame {
        QString id, title;
        QColor color;
        QVector2D pos, size;
    };

    struct SerializedComment {
        QString id, text;
        QVector2D pos;
    };

    DeleteItemsCommand(TextureProjectPtr project,
                       nodegraph::ScenePtr scene,
                       TextureRenderer* renderer,
                       const QList<QString>& nodeIds,
                       const QList<QString>& frameIds,
                       const QList<QString>& commentIds);

    void redo() override;
    void undo() override;

private:
    TextureProjectPtr _project;
    nodegraph::ScenePtr _scene;
    TextureRenderer* _renderer;

    QList<SerializedNode> _nodes;
    QList<SerializedConnection> _connections;
    QList<SerializedFrame> _frames;
    QList<SerializedComment> _comments;

    // Texture-channel (Albedo/Normal/…) assignments that pointed at a deleted
    // node; captured so redo() can drop them and undo() can restore them.
    QMap<TextureChannel, QString> _channelAssignments;
};
