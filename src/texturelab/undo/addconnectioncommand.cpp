#include "addconnectioncommand.h"

#include "../graphics/texturerenderer.h"
#include "graph/scene.h"

AddConnectionCommand::AddConnectionCommand(TextureProjectPtr project,
                                           nodegraph::ScenePtr scene,
                                           TextureRenderer* renderer,
                                           const QString& leftNodeId,
                                           const QString& leftOutput,
                                           const QString& rightNodeId,
                                           const QString& rightInput)
    : QUndoCommand("Connect Nodes")
    , _project(project)
    , _scene(scene)
    , _renderer(renderer)
    , _leftNodeId(leftNodeId)
    , _leftOutput(leftOutput)
    , _rightNodeId(rightNodeId)
    , _rightInput(rightInput)
{}

void AddConnectionCommand::redo()
{
    if (_firstRedo) {
        // Scene already has the connection — just add to project model
        auto left  = _project->getNodeById(_leftNodeId);
        auto right = _project->getNodeById(_rightNodeId);
        if (left && right)
            _project->addConnection(left, right, _rightInput);
        _firstRedo = false;
    } else {
        auto leftG  = _scene->getNodeById(_leftNodeId);
        auto rightG = _scene->getNodeById(_rightNodeId);
        if (leftG && rightG)
            _scene->connectNodes(leftG, _leftOutput, rightG, _rightInput);

        auto left  = _project->getNodeById(_leftNodeId);
        auto right = _project->getNodeById(_rightNodeId);
        if (left && right)
            _project->addConnection(left, right, _rightInput);
    }
    if (_renderer)
        _renderer->update();
}

void AddConnectionCommand::undo()
{
    auto rightG = _scene->getNodeById(_rightNodeId);
    if (rightG) {
        auto port = rightG->getInPortByName(_rightInput);
        if (port && !port->connections.isEmpty())
            _scene->removeConnection(port->connections.first());
    }
    // removeConnection() marks the right node and its downstream chain dirty
    _project->removeConnection(_leftNodeId, _rightNodeId, _rightInput);
    if (_renderer)
        _renderer->update();
}
