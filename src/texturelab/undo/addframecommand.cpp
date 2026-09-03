#include "addframecommand.h"

#include "graph/frame.h"
#include "graph/scene.h"

AddFrameCommand::AddFrameCommand(TextureProjectPtr project,
                                 nodegraph::ScenePtr scene,
                                 const QString& frameId,
                                 QVector2D pos)
    : QUndoCommand("Add Frame")
    , _project(project)
    , _scene(scene)
    , _frameId(frameId)
    , _pos(pos)
{}

void AddFrameCommand::redo()
{
    auto modelFrame   = FramePtr(new Frame());
    modelFrame->id    = _frameId;
    modelFrame->pos   = _pos;
    _project->frames[_frameId] = modelFrame;

    auto gframe = nodegraph::Frame::create();
    gframe->setId(_frameId);
    gframe->setPos(_pos.x(), _pos.y());
    _scene->addFrame(gframe);
}

void AddFrameCommand::undo()
{
    auto gframe = _scene->getFrameById(_frameId);
    if (gframe)
        _scene->removeFrame(gframe);
    _project->frames.remove(_frameId);
}
