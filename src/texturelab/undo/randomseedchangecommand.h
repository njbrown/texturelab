#pragma once

#include "../models.h"
#include <QUndoCommand>

class TextureRenderer;

class RandomSeedChangeCommand : public QUndoCommand {
public:
    RandomSeedChangeCommand(TextureNodePtr node,
                            TextureProjectPtr project,
                            TextureRenderer* renderer,
                            long oldSeed,
                            long newSeed);

    void redo() override;
    void undo() override;

private:
    TextureNodePtr _node;
    TextureProjectPtr _project;
    TextureRenderer* _renderer;
    long _oldSeed, _newSeed;
    bool _firstRedo = true;
};
