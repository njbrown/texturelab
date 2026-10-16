#pragma once

#include "../models.h"
#include "undocommandids.h"
#include <QUndoCommand>

namespace nodegraph {
class Scene;
typedef QSharedPointer<Scene> ScenePtr;
} // namespace nodegraph

class EditCommentCommand : public QUndoCommand {
public:
    EditCommentCommand(CommentPtr comment,
                       TextureProjectPtr project,
                       nodegraph::ScenePtr scene,
                       const QString& oldText,
                       const QString& newText);

    int id() const override { return UndoCommandId::EditComment; }
    bool mergeWith(const QUndoCommand* other) override;

    void redo() override;
    void undo() override;

private:
    void apply(const QString& text);

    // Looked up by _commentId; see EditFrameCommand.
    TextureProjectPtr _project;
    nodegraph::ScenePtr _scene;
    QString _commentId;
    QString _oldText, _newText;
    bool _firstRedo = true;
};
