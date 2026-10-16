#include "projectsettingcommand.h"

ProjectSettingCommand::ProjectSettingCommand(Setting setting, int oldValue,
                                             int newValue,
                                             std::function<void(int)> apply)
    : QUndoCommand(setting == Setting::Resolution ? "Change Resolution"
                                                  : "Change Global Seed")
    , _setting(setting)
    , _oldValue(oldValue)
    , _newValue(newValue)
    , _apply(std::move(apply))
{}

int ProjectSettingCommand::id() const
{
    // Resolution changes are deliberate (and confirmed), so each one is its
    // own step; seed spinbox ticks merge.
    return _setting == Setting::RandomSeed ? UndoCommandId::GlobalSeed : -1;
}

bool ProjectSettingCommand::mergeWith(const QUndoCommand* other)
{
    auto* cmd = static_cast<const ProjectSettingCommand*>(other);
    if (cmd->_setting != _setting ||
        !UndoMerge::withinWindow(_editedAt, cmd->_editedAt))
        return false;
    _newValue = cmd->_newValue;
    _editedAt = cmd->_editedAt;
    return true;
}

void ProjectSettingCommand::redo()
{
    if (_firstRedo) {
        _firstRedo = false;
        return; // already applied by the signal handler
    }
    _apply(_newValue);
}

void ProjectSettingCommand::undo()
{
    _apply(_oldValue);
}
