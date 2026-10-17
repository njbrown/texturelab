#pragma once

#include "undocommandids.h"
#include <QUndoCommand>
#include <functional>

// A change to a project-wide setting (texture resolution or global random
// seed). The caller supplies how to apply a value, so the toolbar widget and
// the model stay in sync on undo/redo. Consecutive seed changes merge into one
// step. First redo is skipped (already applied by the signal handler).
class ProjectSettingCommand : public QUndoCommand {
public:
    enum class Setting { Resolution, RandomSeed };

    ProjectSettingCommand(Setting setting, int oldValue, int newValue,
                          std::function<void(int)> apply);

    int id() const override;
    bool mergeWith(const QUndoCommand* other) override;

    void redo() override;
    void undo() override;

private:
    Setting _setting;
    int _oldValue, _newValue;
    std::function<void(int)> _apply;
    bool _firstRedo = true;
    qint64 _editedAt = UndoMerge::now();
};
