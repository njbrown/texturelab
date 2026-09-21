#include "pastecommand.h"

#include "../clipboard.h"
#include "../graphics/texturerenderer.h"
#include "../telemetry.h"
#include "graph/comment.h"
#include "graph/frame.h"
#include "graph/scene.h"

PasteCommand::PasteCommand(TextureProjectPtr project,
                           nodegraph::ScenePtr scene,
                           TextureRenderer* renderer,
                           QPointF viewCenter)
    : QUndoCommand()
    , _project(project)
    , _scene(scene)
    , _renderer(renderer)
{
    if (!Clipboard::pasteItems(project, viewCenter,
                               _nodes, _connections, _comments, _frames))
        return;

    int total = _nodes.size() + _frames.size() + _comments.size();
    setText(QString("Paste %1 Item%2").arg(total).arg(total == 1 ? "" : "s"));
}

bool PasteCommand::isEmpty() const
{
    return _nodes.isEmpty() && _frames.isEmpty() && _comments.isEmpty();
}

void PasteCommand::addNodeToScene(const TextureNodePtr& node)
{
    auto gnode = nodegraph::Node::create();
    gnode->setId(node->id);
    gnode->setName(node->title);
    for (auto& input : node->inputs)
        gnode->addInPort(input);
    gnode->addOutPort("output");
    gnode->setCenter(node->pos.x(), node->pos.y());
    _scene->addNode(gnode);
}

void PasteCommand::redo()
{
    _scene->clearSelection();

    for (auto& node : _nodes) {
        _project->nodes[node->id] = node;
        addNodeToScene(node);
        auto gnode = _scene->getNodeById(node->id);
        if (gnode)
            gnode->setSelected(true);
    }

    for (auto& con : _connections) {
        // inserted directly (rather than via addConnection) to keep the
        // pasted connection's id stable across undo/redo
        _project->connections[con->id] = con;
        _project->markNodeAsDirty(con->rightNode);
        auto leftG  = _scene->getNodeById(con->leftNode->id);
        auto rightG = _scene->getNodeById(con->rightNode->id);
        if (leftG && rightG)
            _scene->connectNodes(leftG, "output", rightG, con->rightNodeInputName);
    }

    for (auto& comment : _comments) {
        _project->comments[comment->id] = comment;
        auto gc = nodegraph::Comment::create();
        gc->setId(comment->id);
        gc->setText(comment->text);
        gc->setPos(comment->pos.x(), comment->pos.y());
        _scene->addComment(gc);
        gc->setSelected(true);
    }

    for (auto& frame : _frames) {
        _project->frames[frame->id] = frame;
        auto gf = nodegraph::Frame::create();
        gf->setId(frame->id);
        gf->setTitle(frame->text);
        gf->setColor(frame->color);
        gf->setPos(frame->pos.x(), frame->pos.y());
        if (frame->size.x() > 0 && frame->size.y() > 0)
            gf->setSize(frame->size.x(), frame->size.y());
        _scene->addFrame(gf);
        gf->setSelected(true);
    }

    Telemetry::breadcrumb("graph.node", "nodes pasted",
                          {{"pasted_nodes", (int64_t)_nodes.size()},
                           {"node_count", (int64_t)_project->nodes.size()}});
    Telemetry::setTag("project.node_count",
                      std::to_string(_project->nodes.size()));

    if (_renderer)
        _renderer->update();
}

void PasteCommand::undo()
{
    // removeConnection() marks the downstream chain dirty
    for (auto& con : _connections)
        _project->removeConnection(con->id);

    for (auto& node : _nodes) {
        auto gnode = _scene->getNodeById(node->id);
        if (gnode)
            _scene->removeNode(gnode);
        _project->removeNode(node->id);
    }

    for (auto& comment : _comments) {
        auto gc = _scene->getCommentById(comment->id);
        if (gc)
            _scene->removeComment(gc);
        _project->comments.remove(comment->id);
    }

    for (auto& frame : _frames) {
        auto gf = _scene->getFrameById(frame->id);
        if (gf)
            _scene->removeFrame(gf);
        _project->frames.remove(frame->id);
    }

    Telemetry::breadcrumb("graph.node", "paste undone",
                          {{"node_count", (int64_t)_project->nodes.size()}});
    Telemetry::setTag("project.node_count",
                      std::to_string(_project->nodes.size()));

    if (_renderer)
        _renderer->update();
}
