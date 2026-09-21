#pragma once

#include "../models.h"
#include "undocommandids.h"
#include <QMap>
#include <QPointF>
#include <QUndoCommand>

namespace nodegraph {
class Scene;
typedef QSharedPointer<Scene> ScenePtr;
} // namespace nodegraph

class MoveItemsCommand : public QUndoCommand {
public:
    MoveItemsCommand(TextureProjectPtr project,
                     nodegraph::ScenePtr scene,
                     const QMap<QString, QPointF>& oldPositions,
                     const QMap<QString, QPointF>& newPositions);

    int id() const override { return UndoCommandId::MoveItems; }
    bool mergeWith(const QUndoCommand* other) override;

    void redo() override;
    void undo() override;

private:
    void applyPositions(const QMap<QString, QPointF>& positions);

    TextureProjectPtr _project;
    nodegraph::ScenePtr _scene;
    QMap<QString, QPointF> _oldPositions;
    QMap<QString, QPointF> _newPositions;
};
