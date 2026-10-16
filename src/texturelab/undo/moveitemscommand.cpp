#include "moveitemscommand.h"

#include "graph/comment.h"
#include "graph/frame.h"
#include "graph/scene.h"

static QString describe(const nodegraph::LayoutSnapshot& after)
{
    if (after.nodes.isEmpty() && after.frames.isEmpty() &&
        after.comments.isEmpty() && !after.frameSizes.isEmpty())
        return "Resize Frame";
    return "Move Items";
}

MoveItemsCommand::MoveItemsCommand(TextureProjectPtr project,
                                   nodegraph::ScenePtr scene,
                                   const nodegraph::LayoutSnapshot& before,
                                   const nodegraph::LayoutSnapshot& after)
    : QUndoCommand(describe(after))
    , _project(project)
    , _scene(scene)
    , _before(before)
    , _after(after)
{}

void MoveItemsCommand::apply(const nodegraph::LayoutSnapshot& layout)
{
    for (auto it = layout.nodes.begin(); it != layout.nodes.end(); ++it) {
        if (auto gnode = _scene->getNodeById(it.key()))
            gnode->setCenter(it.value().x(), it.value().y());
        if (auto node = _project->getNodeById(it.key()))
            node->pos = QVector2D(it.value());
    }

    for (auto it = layout.frames.begin(); it != layout.frames.end(); ++it) {
        if (auto gframe = _scene->getFrameById(it.key()))
            gframe->setPos(it.value());
        if (auto frame = _project->frames.value(it.key()))
            frame->pos = QVector2D(it.value());
    }

    for (auto it = layout.frameSizes.begin(); it != layout.frameSizes.end();
         ++it) {
        if (auto gframe = _scene->getFrameById(it.key()))
            gframe->setSize(it.value().width(), it.value().height());
        if (auto frame = _project->frames.value(it.key()))
            frame->size = QVector2D(it.value().width(), it.value().height());
    }

    for (auto it = layout.comments.begin(); it != layout.comments.end(); ++it) {
        if (auto gcomment = _scene->getCommentById(it.key()))
            gcomment->setPos(it.value());
        if (auto comment = _project->comments.value(it.key()))
            comment->pos = QVector2D(it.value());
    }
}

void MoveItemsCommand::redo()
{
    apply(_after);
}

void MoveItemsCommand::undo()
{
    apply(_before);
}
