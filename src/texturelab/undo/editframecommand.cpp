#include "editframecommand.h"

#include "graph/frame.h"
#include "graph/scene.h"

EditFrameCommand::EditFrameCommand(FramePtr frame,
                                   nodegraph::ScenePtr scene,
                                   const QString& oldTitle, const QColor& oldColor,
                                   const QString& newTitle, const QColor& newColor)
    : QUndoCommand("Edit Frame")
    , _frame(frame)
    , _scene(scene)
    , _frameId(frame->id)
    , _oldTitle(oldTitle), _newTitle(newTitle)
    , _oldColor(oldColor), _newColor(newColor)
{}

bool EditFrameCommand::mergeWith(const QUndoCommand* other)
{
    auto* cmd = static_cast<const EditFrameCommand*>(other);
    if (cmd->_frameId != _frameId)
        return false;
    _newTitle = cmd->_newTitle;
    _newColor = cmd->_newColor;
    return true;
}

void EditFrameCommand::apply(const QString& title, const QColor& color)
{
    _frame->text  = title;
    _frame->color = color;
    auto gframe   = _scene->getFrameById(_frameId);
    if (gframe) {
        gframe->setTitle(title);
        gframe->setColor(color);
    }
}

void EditFrameCommand::redo()
{
    if (_firstRedo) { _firstRedo = false; return; }
    apply(_newTitle, _newColor);
}

void EditFrameCommand::undo()
{
    apply(_oldTitle, _oldColor);
}
