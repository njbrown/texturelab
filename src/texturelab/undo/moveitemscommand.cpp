#include "moveitemscommand.h"

#include "graph/scene.h"

MoveItemsCommand::MoveItemsCommand(TextureProjectPtr project,
                                   nodegraph::ScenePtr scene,
                                   const QMap<QString, QPointF>& oldPositions,
                                   const QMap<QString, QPointF>& newPositions)
    : QUndoCommand("Move Nodes")
    , _project(project)
    , _scene(scene)
    , _oldPositions(oldPositions)
    , _newPositions(newPositions)
{}

bool MoveItemsCommand::mergeWith(const QUndoCommand* other)
{
    auto* cmd = static_cast<const MoveItemsCommand*>(other);
    if (cmd->_newPositions.keys() != _newPositions.keys())
        return false;
    _newPositions = cmd->_newPositions;
    return true;
}

void MoveItemsCommand::applyPositions(const QMap<QString, QPointF>& positions)
{
    for (auto it = positions.begin(); it != positions.end(); ++it) {
        auto gnode = _scene->getNodeById(it.key());
        if (gnode)
            gnode->setCenter(it.value().x(), it.value().y());
        auto node = _project->getNodeById(it.key());
        if (node)
            node->pos = QVector2D(it.value());
    }
}

void MoveItemsCommand::redo()
{
    applyPositions(_newPositions);
}

void MoveItemsCommand::undo()
{
    applyPositions(_oldPositions);
}
