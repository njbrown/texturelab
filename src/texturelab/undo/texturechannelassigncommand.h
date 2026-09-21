#pragma once

#include "../models.h"
#include <QUndoCommand>
#include <functional>

class TextureChannelAssignCommand : public QUndoCommand {
public:
    TextureChannelAssignCommand(TextureProjectPtr project,
                                TextureChannel channel,
                                const QString& oldNodeId,
                                const QString& newNodeId,
                                std::function<void()> syncViewer);

    void redo() override;
    void undo() override;

private:
    void apply(const QString& nodeId);

    TextureProjectPtr _project;
    TextureChannel _channel;
    QString _oldNodeId, _newNodeId;
    std::function<void()> _syncViewer;
    bool _firstRedo = true;
};
