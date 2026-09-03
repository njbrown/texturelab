#pragma once

#include "../models.h"
#include <QUndoCommand>

class TextureRenderer;

namespace nodegraph {
class Scene;
typedef QSharedPointer<Scene> ScenePtr;
} // namespace nodegraph

// Records a connection drawn in the scene. First redo is a no-op for the scene
// (already created); subsequent redos recreate it.
class AddConnectionCommand : public QUndoCommand {
public:
    AddConnectionCommand(TextureProjectPtr project,
                         nodegraph::ScenePtr scene,
                         TextureRenderer* renderer,
                         const QString& leftNodeId,
                         const QString& leftOutput,
                         const QString& rightNodeId,
                         const QString& rightInput);

    void redo() override;
    void undo() override;

private:
    TextureProjectPtr _project;
    nodegraph::ScenePtr _scene;
    TextureRenderer* _renderer;
    QString _leftNodeId, _leftOutput, _rightNodeId, _rightInput;
    bool _firstRedo = true;
};
