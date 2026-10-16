#include "randomseedchangecommand.h"

#include "../graphics/texturerenderer.h"

RandomSeedChangeCommand::RandomSeedChangeCommand(TextureNodePtr node,
                                                 TextureProjectPtr project,
                                                 TextureRenderer* renderer,
                                                 long oldSeed,
                                                 long newSeed)
    : QUndoCommand("Change Random Seed")
    , _nodeId(node->id)
    , _project(project)
    , _renderer(renderer)
    , _oldSeed(oldSeed)
    , _newSeed(newSeed)
{}

void RandomSeedChangeCommand::redo()
{
    if (_firstRedo) {
        _firstRedo = false;
        return;
    }
    apply(_newSeed);
}

void RandomSeedChangeCommand::undo()
{
    apply(_oldSeed);
}

void RandomSeedChangeCommand::apply(long seed)
{
    auto node = _project->getNodeById(_nodeId);
    if (!node)
        return;
    node->randomSeed = seed;
    _project->markNodeAsDirty(node);
    if (_renderer)
        _renderer->update();
}
