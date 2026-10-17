#pragma once

#include "../models.h"
#include "undocommandids.h"
#include <QUndoCommand>

class TextureRenderer;

class RandomSeedChangeCommand : public QUndoCommand {
public:
    RandomSeedChangeCommand(TextureNodePtr node,
                            TextureProjectPtr project,
                            TextureRenderer* renderer,
                            long oldSeed,
                            long newSeed);

    int id() const override { return UndoCommandId::NodeSeed; }
    bool mergeWith(const QUndoCommand* other) override;

    void redo() override;
    void undo() override;

private:
    void apply(long seed);

    QString _nodeId; // by id: a delete/undo recreates the node object
    TextureProjectPtr _project;
    TextureRenderer* _renderer;
    long _oldSeed, _newSeed;
    bool _firstRedo = true;
    qint64 _editedAt = UndoMerge::now();
};
