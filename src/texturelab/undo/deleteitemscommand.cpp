#include "deleteitemscommand.h"

#include "../graphics/texturerenderer.h"
#include "../libraries/library.h"
#include "../props.h"
#include "../telemetry.h"
#include "graph/comment.h"
#include "graph/frame.h"
#include "graph/scene.h"

static void addNodeToScene(nodegraph::ScenePtr scene, const TextureNodePtr& node)
{
    auto gnode = nodegraph::Node::create();
    gnode->setId(node->id);
    gnode->setName(node->title);
    for (auto& input : node->inputs)
        gnode->addInPort(input);
    gnode->addOutPort("output");
    gnode->setCenter(node->pos.x(), node->pos.y());
    scene->addNode(gnode);
}

DeleteItemsCommand::DeleteItemsCommand(TextureProjectPtr project,
                                       nodegraph::ScenePtr scene,
                                       TextureRenderer* renderer,
                                       const QList<QString>& nodeIds,
                                       const QList<QString>& frameIds,
                                       const QList<QString>& commentIds)
    : QUndoCommand()
    , _project(project)
    , _scene(scene)
    , _renderer(renderer)
{
    int total = nodeIds.size() + frameIds.size() + commentIds.size();
    setText(QString("Delete %1 Item%2").arg(total).arg(total == 1 ? "" : "s"));

    QSet<QString> deletedNodeIds(nodeIds.begin(), nodeIds.end());

    // Capture any texture-channel assignments pointing at a deleted node so we
    // can remove them in redo() and restore them in undo(). Left behind, a
    // stale id here is later looked up by passTextureChannelsToViewer3D().
    for (auto it = _project->textureChannels.constBegin();
         it != _project->textureChannels.constEnd(); ++it) {
        if (deletedNodeIds.contains(it.value()))
            _channelAssignments.insert(it.key(), it.value());
    }

    for (const auto& id : nodeIds) {
        auto node = _project->getNodeById(id);
        if (!node)
            continue;
        SerializedNode sn;
        sn.typeName   = node->typeName;
        sn.id         = node->id;
        sn.exportName = node->exportName;
        sn.randomSeed = node->randomSeed;
        sn.pos        = node->pos;
        auto gnode = _scene->getNodeById(id);
        if (gnode)
            sn.pos = QVector2D(gnode->getCenter());
        for (auto key : node->props.keys())
            sn.props[key] = node->props[key]->toJsonValue();
        _nodes.append(sn);
    }

    for (const auto& con : _project->connections) {
        if (deletedNodeIds.contains(con->leftNode->id) ||
            deletedNodeIds.contains(con->rightNode->id)) {
            SerializedConnection sc;
            sc.id          = con->id;
            sc.leftNodeId  = con->leftNode->id;
            sc.leftOutput  = con->leftNodeOutputName.isEmpty() ? "output" : con->leftNodeOutputName;
            sc.rightNodeId = con->rightNode->id;
            sc.rightInput  = con->rightNodeInputName;
            _connections.append(sc);
        }
    }

    for (const auto& id : frameIds) {
        auto frame = _project->frames.value(id);
        if (!frame)
            continue;
        SerializedFrame sf;
        sf.id    = frame->id;
        sf.title = frame->text;
        sf.color = frame->color;
        sf.pos   = frame->pos;
        sf.size  = frame->size;
        auto gframe = _scene->getFrameById(id);
        if (gframe) {
            sf.pos      = QVector2D(gframe->pos());
            auto sz     = gframe->frameRect().size();
            sf.size     = QVector2D(sz.width(), sz.height());
        }
        _frames.append(sf);
    }

    for (const auto& id : commentIds) {
        auto comment = _project->comments.value(id);
        if (!comment)
            continue;
        SerializedComment sc;
        sc.id   = comment->id;
        sc.text = comment->text;
        sc.pos  = comment->pos;
        auto gcomment = _scene->getCommentById(id);
        if (gcomment)
            sc.pos = QVector2D(gcomment->pos());
        _comments.append(sc);
    }
}

void DeleteItemsCommand::redo()
{
    // removeConnection() marks each right node and its downstream chain dirty
    for (const auto& sc : _connections)
        _project->removeConnection(sc.leftNodeId, sc.rightNodeId, sc.rightInput);

    for (const auto& sn : _nodes) {
        auto gnode = _scene->getNodeById(sn.id);
        if (gnode)
            _scene->removeNode(gnode);
        _project->nodes.remove(sn.id);
    }

    for (const auto& sf : _frames) {
        auto gframe = _scene->getFrameById(sf.id);
        if (gframe)
            _scene->removeFrame(gframe);
        _project->frames.remove(sf.id);
    }

    for (const auto& sc : _comments) {
        auto gcomment = _scene->getCommentById(sc.id);
        if (gcomment)
            _scene->removeComment(gcomment);
        _project->comments.remove(sc.id);
    }

    // Drop channel assignments that referenced the now-deleted nodes.
    for (auto it = _channelAssignments.constBegin();
         it != _channelAssignments.constEnd(); ++it) {
        _project->textureChannels.remove(it.key());
    }

    Telemetry::breadcrumb("graph.node", "items deleted",
                          {{"deleted_nodes", (int64_t)_nodes.size()},
                           {"node_count", (int64_t)_project->nodes.size()}});
    Telemetry::setTag("project.node_count",
                      std::to_string(_project->nodes.size()));

    if (_renderer)
        _renderer->update();
}

void DeleteItemsCommand::undo()
{
    for (const auto& sn : _nodes) {
        auto node = _project->library->createNode(sn.typeName);
        node->id         = sn.id;
        node->exportName = sn.exportName;
        node->randomSeed = sn.randomSeed;
        node->pos        = sn.pos;
        node->isDirty    = true;
        for (auto key : sn.props.keys()) {
            auto prop = node->getProp(key);
            if (prop)
                prop->fromJsonValue(sn.props[key]);
        }
        _project->addNode(node);
        addNodeToScene(_scene, node);
    }

    for (const auto& sc : _connections) {
        auto leftNode  = _project->getNodeById(sc.leftNodeId);
        auto rightNode = _project->getNodeById(sc.rightNodeId);
        if (!leftNode || !rightNode)
            continue;
        _project->addConnection(leftNode, rightNode, sc.rightInput);
        auto leftG  = _scene->getNodeById(sc.leftNodeId);
        auto rightG = _scene->getNodeById(sc.rightNodeId);
        if (leftG && rightG)
            _scene->connectNodes(leftG, sc.leftOutput, rightG, sc.rightInput);
    }

    for (const auto& sf : _frames) {
        auto modelFrame   = FramePtr(new Frame());
        modelFrame->id    = sf.id;
        modelFrame->text  = sf.title;
        modelFrame->color = sf.color;
        modelFrame->pos   = sf.pos;
        modelFrame->size  = sf.size;
        _project->frames[sf.id] = modelFrame;

        auto gframe = nodegraph::Frame::create();
        gframe->setId(sf.id);
        gframe->setTitle(sf.title);
        gframe->setColor(sf.color);
        gframe->setPos(sf.pos.x(), sf.pos.y());
        if (sf.size.x() > 0 && sf.size.y() > 0)
            gframe->setSize(sf.size.x(), sf.size.y());
        _scene->addFrame(gframe);
    }

    for (const auto& sc : _comments) {
        auto modelComment   = CommentPtr(new Comment());
        modelComment->id    = sc.id;
        modelComment->text  = sc.text;
        modelComment->pos   = sc.pos;
        _project->comments[sc.id] = modelComment;

        auto gcomment = nodegraph::Comment::create();
        gcomment->setId(sc.id);
        gcomment->setText(sc.text);
        gcomment->setPos(sc.pos.x(), sc.pos.y());
        _scene->addComment(gcomment);
    }

    // Restore channel assignments now that the nodes they reference exist again.
    for (auto it = _channelAssignments.constBegin();
         it != _channelAssignments.constEnd(); ++it) {
        _project->textureChannels.insert(it.key(), it.value());
    }

    Telemetry::breadcrumb("graph.node", "delete undone",
                          {{"restored_nodes", (int64_t)_nodes.size()},
                           {"node_count", (int64_t)_project->nodes.size()}});
    Telemetry::setTag("project.node_count",
                      std::to_string(_project->nodes.size()));

    if (_renderer)
        _renderer->update();
}
