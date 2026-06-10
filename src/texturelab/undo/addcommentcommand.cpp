#include "addcommentcommand.h"

#include "graph/comment.h"
#include "graph/scene.h"

AddCommentCommand::AddCommentCommand(TextureProjectPtr project,
                                     nodegraph::ScenePtr scene,
                                     const QString& commentId,
                                     QVector2D pos)
    : QUndoCommand("Add Comment")
    , _project(project)
    , _scene(scene)
    , _commentId(commentId)
    , _pos(pos)
{}

void AddCommentCommand::redo()
{
    auto modelComment   = CommentPtr(new Comment());
    modelComment->id    = _commentId;
    modelComment->pos   = _pos;
    _project->comments[_commentId] = modelComment;

    auto gcomment = nodegraph::Comment::create();
    gcomment->setId(_commentId);
    gcomment->setPos(_pos.x(), _pos.y());
    _scene->addComment(gcomment);
}

void AddCommentCommand::undo()
{
    auto gcomment = _scene->getCommentById(_commentId);
    if (gcomment)
        _scene->removeComment(gcomment);
    _project->comments.remove(_commentId);
}
