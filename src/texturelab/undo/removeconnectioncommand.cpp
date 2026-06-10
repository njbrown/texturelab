#include "removeconnectioncommand.h"

#include "../graphics/texturerenderer.h"
#include "graph/scene.h"

RemoveConnectionCommand::RemoveConnectionCommand(TextureProjectPtr project,
                                                 nodegraph::ScenePtr scene,
                                                 TextureRenderer* renderer,
                                                 const QString& leftNodeId,
                                                 const QString& leftOutput,
                                                 const QString& rightNodeId,
                                                 const QString& rightInput)
    : QUndoCommand("Remove Connection")
    , _project(project)
    , _scene(scene)
    , _renderer(renderer)
    , _leftNodeId(leftNodeId)
    , _leftOutput(leftOutput)
    , _rightNodeId(rightNodeId)
    , _rightInput(rightInput)
{}

void RemoveConnectionCommand::redo()
{
    if (_firstRedo) {
        // Scene already removed it — just remove from project model
        auto con = _project->removeConnection(_leftNodeId, _rightNodeId, _rightInput);
        if (con && con->rightNode)
            con->rightNode->isDirty = true;
        _firstRedo = false;
    } else {
        auto rightG = _scene->getNodeById(_rightNodeId);
        if (rightG) {
            auto port = rightG->getInPortByName(_rightInput);
            if (port && !port->connections.isEmpty())
                _scene->removeConnection(port->connections.first());
        }
        auto con = _project->removeConnection(_leftNodeId, _rightNodeId, _rightInput);
        if (con && con->rightNode)
            con->rightNode->isDirty = true;
    }
    if (_renderer)
        _renderer->update();
}

void RemoveConnectionCommand::undo()
{
    auto leftG  = _scene->getNodeById(_leftNodeId);
    auto rightG = _scene->getNodeById(_rightNodeId);
    if (leftG && rightG)
        _scene->connectNodes(leftG, _leftOutput, rightG, _rightInput);

    auto left  = _project->getNodeById(_leftNodeId);
    auto right = _project->getNodeById(_rightNodeId);
    if (left && right) {
        _project->addConnection(left, right, _rightInput);
        right->isDirty = true;
    }
    if (_renderer)
        _renderer->update();
}
