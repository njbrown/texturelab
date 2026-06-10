#pragma once

#include "../models.h"
#include "undocommandids.h"
#include <QUndoCommand>
#include <QVariant>

class TextureRenderer;

// A single node property value change. Consecutive changes to the same prop
// merge into one undo step. First redo is skipped (already applied by the
// signal handler).
class PropertyChangeCommand : public QUndoCommand {
public:
    PropertyChangeCommand(TextureNodePtr node,
                          TextureProjectPtr project,
                          TextureRenderer* renderer,
                          const QString& propName,
                          QVariant oldValue,
                          QVariant newValue);

    int id() const override { return UndoCommandId::PropertyChange; }
    bool mergeWith(const QUndoCommand* other) override;

    void redo() override;
    void undo() override;

private:
    void applyValue(const QVariant& value);

    TextureNodePtr _node;
    TextureProjectPtr _project;
    TextureRenderer* _renderer;
    QString _propName;
    QVariant _oldValue;
    QVariant _newValue;
    bool _firstRedo = true;
};
