#pragma once

#include "../models.h"
#include "undocommandids.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QUndoCommand>

class TextureRenderer;

// Any change to texture variables or their presets: create/delete,
// link/unlink, rename, bounds, options, reorder, value, preset save/apply/
// rename/delete. Restores a snapshot of the variables, presets and active
// preset, plus the values of every node prop the change wrote, which keeps
// variable state and linked props consistent through undo/redo. Build one with
// VariablesEdit rather than directly.
//
// Value scrubs carry a merge key (the variable id) and merge like
// PropertyChangeCommand. First redo is skipped: the change is already applied.
class VariablesChangeCommand : public QUndoCommand {
public:
    // "nodeId/propName" -> prop value (Prop::toJsonValue)
    using PropValues = QMap<QString, QJsonValue>;

    VariablesChangeCommand(TextureProjectPtr project,
                           TextureRenderer* renderer,
                           const QString& text,
                           const QString& mergeKey,
                           QJsonObject before,
                           QJsonObject after,
                           PropValues propsBefore,
                           PropValues propsAfter);

    int id() const override
    {
        return _mergeKey.isEmpty() ? -1 : UndoCommandId::VariablesChange;
    }
    bool mergeWith(const QUndoCommand* other) override;

    void redo() override;
    void undo() override;

private:
    void apply(const QJsonObject& state, const PropValues& props);

    TextureProjectPtr _project;
    TextureRenderer* _renderer;
    QString _mergeKey;
    QJsonObject _before;
    QJsonObject _after;
    PropValues _propsBefore;
    PropValues _propsAfter;
    bool _firstRedo = true;
    qint64 _editedAt = UndoMerge::now();
};

// Captures the state around a variables change:
//
//     VariablesEdit edit(project, renderer, "Change Roughness", var->id);
//     edit.touchVariable(var);          // before mutating
//     var->setValue(v);
//     project->applyVariable(var);
//     undoStack->push(edit.finish());
class VariablesEdit {
public:
    VariablesEdit(TextureProjectPtr project,
                  TextureRenderer* renderer,
                  const QString& text,
                  const QString& mergeKey = QString());

    // Record a prop's value before the change writes it.
    void touchProp(const QString& nodeId, const QString& propName);
    // touchProp() for every prop linked to the variable.
    void touchVariable(const TextureVariablePtr& var);

    // Snapshot the after-state and build the command (caller owns it).
    VariablesChangeCommand* finish();

private:
    TextureProjectPtr _project;
    TextureRenderer* _renderer;
    QString _text;
    QString _mergeKey;
    QJsonObject _before;
    VariablesChangeCommand::PropValues _propsBefore;
};

// The variables state a VariablesChangeCommand snapshots and restores.
QJsonObject variablesSnapshot(const TextureProjectPtr& project);
void restoreVariablesSnapshot(const TextureProjectPtr& project,
                              const QJsonObject& state);
