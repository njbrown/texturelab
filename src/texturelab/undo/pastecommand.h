#pragma once

#include "../models.h"
#include <QPointF>
#include <QUndoCommand>

class TextureRenderer;

namespace nodegraph {
class Scene;
typedef QSharedPointer<Scene> ScenePtr;
} // namespace nodegraph

class PasteCommand : public QUndoCommand {
public:
    PasteCommand(TextureProjectPtr project,
                 nodegraph::ScenePtr scene,
                 TextureRenderer* renderer,
                 QPointF viewCenter);

    void redo() override;
    void undo() override;

    bool isEmpty() const;

private:
    void addNodeToScene(const TextureNodePtr& node);

    TextureProjectPtr _project;
    nodegraph::ScenePtr _scene;
    TextureRenderer* _renderer;

    QList<TextureNodePtr> _nodes;
    QList<ConnectionPtr> _connections;
    QList<CommentPtr> _comments;
    QList<FramePtr> _frames;
};
