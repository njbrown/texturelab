#pragma once

#include "../models.h"
#include <QUndoCommand>
#include <QVector2D>

namespace nodegraph {
class Scene;
typedef QSharedPointer<Scene> ScenePtr;
} // namespace nodegraph

class AddFrameCommand : public QUndoCommand {
public:
    AddFrameCommand(TextureProjectPtr project,
                    nodegraph::ScenePtr scene,
                    const QString& frameId,
                    QVector2D pos);

    void redo() override;
    void undo() override;

private:
    TextureProjectPtr _project;
    nodegraph::ScenePtr _scene;
    QString _frameId;
    QVector2D _pos;
};
