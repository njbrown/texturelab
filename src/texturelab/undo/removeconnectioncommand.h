#pragma once

#include "../models.h"
#include <QUndoCommand>

class TextureRenderer;

namespace nodegraph {
class Scene;
typedef QSharedPointer<Scene> ScenePtr;
} // namespace nodegraph

// Records a connection removed in the scene. First redo only removes from the
// project model (scene already done).
class RemoveConnectionCommand : public QUndoCommand {
public:
    RemoveConnectionCommand(TextureProjectPtr project,
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
