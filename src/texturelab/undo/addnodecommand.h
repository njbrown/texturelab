#pragma once

#include "../models.h"
#include <QUndoCommand>
#include <QVector2D>

class TextureRenderer;

namespace nodegraph {
class Scene;
typedef QSharedPointer<Scene> ScenePtr;
} // namespace nodegraph

class AddNodeCommand : public QUndoCommand {
public:
    AddNodeCommand(TextureProjectPtr project,
                   nodegraph::ScenePtr scene,
                   TextureRenderer* renderer,
                   const QString& typeName,
                   QVector2D pos);

    void redo() override;
    void undo() override;

private:
    TextureProjectPtr _project;
    nodegraph::ScenePtr _scene;
    TextureRenderer* _renderer;
    QString _typeName;
    QVector2D _pos;
    QString _nodeId; // generated in constructor, reused on re-redo
};
