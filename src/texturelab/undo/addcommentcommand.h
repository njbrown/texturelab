#pragma once

#include "../models.h"
#include <QUndoCommand>
#include <QVector2D>

namespace nodegraph {
class Scene;
typedef QSharedPointer<Scene> ScenePtr;
} // namespace nodegraph

class AddCommentCommand : public QUndoCommand {
public:
    AddCommentCommand(TextureProjectPtr project,
                      nodegraph::ScenePtr scene,
                      const QString& commentId,
                      QVector2D pos);

    void redo() override;
    void undo() override;

private:
    TextureProjectPtr _project;
    nodegraph::ScenePtr _scene;
    QString _commentId;
    QVector2D _pos;
};
