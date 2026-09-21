#include "texturechannelassigncommand.h"

TextureChannelAssignCommand::TextureChannelAssignCommand(
    TextureProjectPtr project,
    TextureChannel channel,
    const QString& oldNodeId,
    const QString& newNodeId,
    std::function<void()> syncViewer)
    : QUndoCommand("Assign Texture Channel")
    , _project(project)
    , _channel(channel)
    , _oldNodeId(oldNodeId)
    , _newNodeId(newNodeId)
    , _syncViewer(syncViewer)
{}

void TextureChannelAssignCommand::apply(const QString& nodeId)
{
    if (nodeId.isEmpty())
        _project->textureChannels.remove(_channel);
    else
        _project->textureChannels[_channel] = nodeId;
    if (_syncViewer)
        _syncViewer();
}

void TextureChannelAssignCommand::redo()
{
    if (_firstRedo) { _firstRedo = false; return; }
    apply(_newNodeId);
}

void TextureChannelAssignCommand::undo()
{
    apply(_oldNodeId);
}
