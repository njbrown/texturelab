#include "randomseedchangecommand.h"

#include "../graphics/texturerenderer.h"

RandomSeedChangeCommand::RandomSeedChangeCommand(TextureNodePtr node,
                                                 TextureProjectPtr project,
                                                 TextureRenderer* renderer,
                                                 long oldSeed,
                                                 long newSeed)
    : QUndoCommand("Change Random Seed")
    , _node(node)
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
    _node->randomSeed = _newSeed;
    _project->markNodeAsDirty(_node);
    if (_renderer)
        _renderer->update();
}

void RandomSeedChangeCommand::undo()
{
    _node->randomSeed = _oldSeed;
    _project->markNodeAsDirty(_node);
    if (_renderer)
        _renderer->update();
}
