#pragma once

#include "../models.h"
#include "nodegraph.h"
#include <QUndoCommand>

namespace nodegraph {
class Scene;
typedef QSharedPointer<Scene> ScenePtr;
} // namespace nodegraph

// One mouse gesture's worth of layout change: node moves, frame moves and
// resizes, comment moves. First redo is a no-op re-apply of what the drag
// already did. Deliberately never merges: each drag is its own undo step.
class MoveItemsCommand : public QUndoCommand {
public:
    MoveItemsCommand(TextureProjectPtr project,
                     nodegraph::ScenePtr scene,
                     const nodegraph::LayoutSnapshot& before,
                     const nodegraph::LayoutSnapshot& after);

    void redo() override;
    void undo() override;

private:
    void apply(const nodegraph::LayoutSnapshot& layout);

    TextureProjectPtr _project;
    nodegraph::ScenePtr _scene;
    nodegraph::LayoutSnapshot _before;
    nodegraph::LayoutSnapshot _after;
};
