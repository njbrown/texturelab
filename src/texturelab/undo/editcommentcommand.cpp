#include "editcommentcommand.h"

#include "graph/comment.h"
#include "graph/scene.h"

EditCommentCommand::EditCommentCommand(CommentPtr comment,
                                       nodegraph::ScenePtr scene,
                                       const QString& oldText,
                                       const QString& newText)
    : QUndoCommand("Edit Comment")
    , _comment(comment)
    , _scene(scene)
    , _commentId(comment->id)
    , _oldText(oldText)
    , _newText(newText)
{}

bool EditCommentCommand::mergeWith(const QUndoCommand* other)
{
    auto* cmd = static_cast<const EditCommentCommand*>(other);
    if (cmd->_commentId != _commentId)
        return false;
    _newText = cmd->_newText;
    return true;
}

void EditCommentCommand::apply(const QString& text)
{
    _comment->text = text;
    auto gcomment  = _scene->getCommentById(_commentId);
    if (gcomment)
        gcomment->setText(text);
}

void EditCommentCommand::redo()
{
    if (_firstRedo) { _firstRedo = false; return; }
    apply(_newText);
}

void EditCommentCommand::undo()
{
    apply(_oldText);
}
