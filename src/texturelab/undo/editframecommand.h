#pragma once

#include "../models.h"
#include "undocommandids.h"
#include <QColor>
#include <QUndoCommand>

namespace nodegraph {
class Scene;
typedef QSharedPointer<Scene> ScenePtr;
} // namespace nodegraph

class EditFrameCommand : public QUndoCommand {
public:
    EditFrameCommand(FramePtr frame,
                     nodegraph::ScenePtr scene,
                     const QString& oldTitle, const QColor& oldColor,
                     const QString& newTitle, const QColor& newColor);

    int id() const override { return UndoCommandId::EditFrame; }
    bool mergeWith(const QUndoCommand* other) override;

    void redo() override;
    void undo() override;

private:
    void apply(const QString& title, const QColor& color);

    FramePtr _frame;
    nodegraph::ScenePtr _scene;
    QString _frameId;
    QString _oldTitle, _newTitle;
    QColor _oldColor, _newColor;
    bool _firstRedo = true;
};
