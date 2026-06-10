#include "undocommands.h"

#include "../clipboard.h"
#include "../graphics/texturerenderer.h"
#include "../libraries/library.h"
#include "../props.h"
#include "graph/comment.h"
#include "graph/frame.h"
#include "graph/scene.h"

#include <QUuid>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static QString newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// Add a TextureNode to the nodegraph scene (mirrors GraphWidget::addNode)
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

// ---------------------------------------------------------------------------
// AddNodeCommand
// ---------------------------------------------------------------------------

AddNodeCommand::AddNodeCommand(TextureProjectPtr project,
                               nodegraph::ScenePtr scene,
                               TextureRenderer* renderer,
                               const QString& typeName,
                               QVector2D pos)
    : QUndoCommand(QString("Add %1 Node").arg(typeName))
    , _project(project)
    , _scene(scene)
    , _renderer(renderer)
    , _typeName(typeName)
    , _pos(pos)
    , _nodeId(newId())
{}

void AddNodeCommand::redo()
{
    auto node = _project->library->createNode(_typeName);
    node->id = _nodeId;
    node->pos = _pos;
    _project->addNode(node);
    addNodeToScene(_scene, node);
    if (_renderer)
        _renderer->update();
}

void AddNodeCommand::undo()
{
    auto sceneNode = _scene->getNodeById(_nodeId);
    if (sceneNode)
        _scene->removeNode(sceneNode);

    // Remove all connections involving this node from the project
    for (auto key : _project->connections.keys()) {
        auto con = _project->connections.value(key);
        if (con->leftNode->id == _nodeId || con->rightNode->id == _nodeId) {
            if (con->leftNode->id == _nodeId)
                con->rightNode->isDirty = true;
            _project->connections.remove(key);
        }
    }

    _project->nodes.remove(_nodeId);
    if (_renderer)
        _renderer->update();
}

// ---------------------------------------------------------------------------
// DeleteItemsCommand
// ---------------------------------------------------------------------------

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
    // Determine display text
    int total = nodeIds.size() + frameIds.size() + commentIds.size();
    setText(QString("Delete %1 Item%2").arg(total).arg(total == 1 ? "" : "s"));

    QSet<QString> deletedNodeIds(nodeIds.begin(), nodeIds.end());

    // Serialize nodes
    for (const auto& id : nodeIds) {
        auto node = _project->getNodeById(id);
        if (!node)
            continue;
        SerializedNode sn;
        sn.typeName  = node->typeName;
        sn.id        = node->id;
        sn.exportName = node->exportName;
        sn.randomSeed = node->randomSeed;
        sn.pos       = node->pos;
        // Sync visual position before serializing
        auto gnode = _scene->getNodeById(id);
        if (gnode)
            sn.pos = QVector2D(gnode->getCenter());
        for (auto key : node->props.keys())
            sn.props[key] = node->props[key]->toJsonValue();
        _nodes.append(sn);
    }

    // Serialize all connections touching any deleted node
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

    // Serialize frames
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
        // Sync visual position/size
        auto gframe = _scene->getFrameById(id);
        if (gframe) {
            sf.pos  = QVector2D(gframe->pos());
            auto sz = gframe->frameRect().size();
            sf.size = QVector2D(sz.width(), sz.height());
        }
        _frames.append(sf);
    }

    // Serialize comments
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
    // Remove project connections first (mark downstream nodes dirty)
    for (const auto& sc : _connections) {
        auto con = _project->removeConnection(sc.leftNodeId, sc.rightNodeId, sc.rightInput);
        if (con && con->rightNode)
            con->rightNode->isDirty = true;
    }

    // Remove nodes (scene::removeNode also removes scene-level connections)
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

    if (_renderer)
        _renderer->update();
}

void DeleteItemsCommand::undo()
{
    // Recreate nodes in project + scene
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

    // Recreate connections in project + scene
    for (const auto& sc : _connections) {
        auto leftNode  = _project->getNodeById(sc.leftNodeId);
        auto rightNode = _project->getNodeById(sc.rightNodeId);
        if (!leftNode || !rightNode)
            continue;

        _project->addConnection(leftNode, rightNode, sc.rightInput);
        rightNode->isDirty = true;

        auto leftGNode  = _scene->getNodeById(sc.leftNodeId);
        auto rightGNode = _scene->getNodeById(sc.rightNodeId);
        if (leftGNode && rightGNode)
            _scene->connectNodes(leftGNode, sc.leftOutput, rightGNode, sc.rightInput);
    }

    // Recreate frames
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

    // Recreate comments
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

    if (_renderer)
        _renderer->update();
}

// ---------------------------------------------------------------------------
// AddConnectionCommand
// ---------------------------------------------------------------------------

AddConnectionCommand::AddConnectionCommand(TextureProjectPtr project,
                                           nodegraph::ScenePtr scene,
                                           TextureRenderer* renderer,
                                           const QString& leftNodeId,
                                           const QString& leftOutput,
                                           const QString& rightNodeId,
                                           const QString& rightInput)
    : QUndoCommand(QString("Connect Nodes"))
    , _project(project)
    , _scene(scene)
    , _renderer(renderer)
    , _leftNodeId(leftNodeId)
    , _leftOutput(leftOutput)
    , _rightNodeId(rightNodeId)
    , _rightInput(rightInput)
{}

void AddConnectionCommand::redo()
{
    if (_firstRedo) {
        // Scene already has the connection — just add to project model
        auto left  = _project->getNodeById(_leftNodeId);
        auto right = _project->getNodeById(_rightNodeId);
        if (left && right) {
            _project->addConnection(left, right, _rightInput);
            right->isDirty = true;
        }
        _firstRedo = false;
    } else {
        // Recreate in scene + project
        auto leftG  = _scene->getNodeById(_leftNodeId);
        auto rightG = _scene->getNodeById(_rightNodeId);
        if (leftG && rightG)
            _scene->connectNodes(leftG, _leftOutput, rightG, _rightInput);

        auto left  = _project->getNodeById(_leftNodeId);
        auto right = _project->getNodeById(_rightNodeId);
        if (left && right) {
            _project->addConnection(left, right, _rightInput);
            right->isDirty = true;
        }
    }
    if (_renderer)
        _renderer->update();
}

void AddConnectionCommand::undo()
{
    // Remove from scene
    auto rightG = _scene->getNodeById(_rightNodeId);
    if (rightG) {
        auto port = rightG->getInPortByName(_rightInput);
        if (port && !port->connections.isEmpty())
            _scene->removeConnection(port->connections.first());
    }
    // Remove from project
    auto right = _project->getNodeById(_rightNodeId);
    if (right)
        right->isDirty = true;
    _project->removeConnection(_leftNodeId, _rightNodeId, _rightInput);
    if (_renderer)
        _renderer->update();
}

// ---------------------------------------------------------------------------
// RemoveConnectionCommand
// ---------------------------------------------------------------------------

RemoveConnectionCommand::RemoveConnectionCommand(TextureProjectPtr project,
                                                 nodegraph::ScenePtr scene,
                                                 TextureRenderer* renderer,
                                                 const QString& leftNodeId,
                                                 const QString& leftOutput,
                                                 const QString& rightNodeId,
                                                 const QString& rightInput)
    : QUndoCommand("Remove Connection")
    , _project(project)
    , _scene(scene)
    , _renderer(renderer)
    , _leftNodeId(leftNodeId)
    , _leftOutput(leftOutput)
    , _rightNodeId(rightNodeId)
    , _rightInput(rightInput)
{}

void RemoveConnectionCommand::redo()
{
    if (_firstRedo) {
        // Scene already removed it — just remove from project model
        auto con = _project->removeConnection(_leftNodeId, _rightNodeId, _rightInput);
        if (con && con->rightNode)
            con->rightNode->isDirty = true;
        _firstRedo = false;
    } else {
        // Remove from scene
        auto rightG = _scene->getNodeById(_rightNodeId);
        if (rightG) {
            auto port = rightG->getInPortByName(_rightInput);
            if (port && !port->connections.isEmpty())
                _scene->removeConnection(port->connections.first());
        }
        // Remove from project
        auto con = _project->removeConnection(_leftNodeId, _rightNodeId, _rightInput);
        if (con && con->rightNode)
            con->rightNode->isDirty = true;
    }
    if (_renderer)
        _renderer->update();
}

void RemoveConnectionCommand::undo()
{
    // Restore in scene
    auto leftG  = _scene->getNodeById(_leftNodeId);
    auto rightG = _scene->getNodeById(_rightNodeId);
    if (leftG && rightG)
        _scene->connectNodes(leftG, _leftOutput, rightG, _rightInput);

    // Restore in project
    auto left  = _project->getNodeById(_leftNodeId);
    auto right = _project->getNodeById(_rightNodeId);
    if (left && right) {
        _project->addConnection(left, right, _rightInput);
        right->isDirty = true;
    }
    if (_renderer)
        _renderer->update();
}

// ---------------------------------------------------------------------------
// MoveItemsCommand
// ---------------------------------------------------------------------------

MoveItemsCommand::MoveItemsCommand(TextureProjectPtr project,
                                   nodegraph::ScenePtr scene,
                                   const QMap<QString, QPointF>& oldPositions,
                                   const QMap<QString, QPointF>& newPositions)
    : QUndoCommand("Move Nodes")
    , _project(project)
    , _scene(scene)
    , _oldPositions(oldPositions)
    , _newPositions(newPositions)
{}

bool MoveItemsCommand::mergeWith(const QUndoCommand* other)
{
    auto* cmd = static_cast<const MoveItemsCommand*>(other);
    if (cmd->_newPositions.keys() != _newPositions.keys())
        return false;
    _newPositions = cmd->_newPositions;
    return true;
}

void MoveItemsCommand::applyPositions(const QMap<QString, QPointF>& positions)
{
    for (auto it = positions.begin(); it != positions.end(); ++it) {
        auto gnode = _scene->getNodeById(it.key());
        if (gnode)
            gnode->setCenter(it.value().x(), it.value().y());
        auto node = _project->getNodeById(it.key());
        if (node)
            node->pos = QVector2D(it.value());
    }
}

void MoveItemsCommand::redo()
{
    applyPositions(_newPositions);
}

void MoveItemsCommand::undo()
{
    applyPositions(_oldPositions);
}

// ---------------------------------------------------------------------------
// PropertyChangeCommand
// ---------------------------------------------------------------------------

PropertyChangeCommand::PropertyChangeCommand(TextureNodePtr node,
                                             TextureProjectPtr project,
                                             TextureRenderer* renderer,
                                             const QString& propName,
                                             QVariant oldValue,
                                             QVariant newValue)
    : QUndoCommand(QString("Change %1").arg(propName))
    , _node(node)
    , _project(project)
    , _renderer(renderer)
    , _propName(propName)
    , _oldValue(oldValue)
    , _newValue(newValue)
{}

bool PropertyChangeCommand::mergeWith(const QUndoCommand* other)
{
    auto* cmd = static_cast<const PropertyChangeCommand*>(other);
    if (cmd->_node != _node || cmd->_propName != _propName)
        return false;
    _newValue = cmd->_newValue;
    return true;
}

void PropertyChangeCommand::applyValue(const QVariant& value)
{
    _node->setProp(_propName, value);
    _project->markNodeAsDirty(_node);
    if (_renderer)
        _renderer->update();
}

void PropertyChangeCommand::redo()
{
    if (_firstRedo) {
        _firstRedo = false;
        return; // already applied by the signal handler
    }
    applyValue(_newValue);
}

void PropertyChangeCommand::undo()
{
    applyValue(_oldValue);
}

// ---------------------------------------------------------------------------
// RandomSeedChangeCommand
// ---------------------------------------------------------------------------

RandomSeedChangeCommand::RandomSeedChangeCommand(TextureNodePtr node,
                                                 TextureProjectPtr project,
                                                 TextureRenderer* renderer,
                                                 long oldSeed,
                                                 long newSeed)
    : QUndoCommand("Change Random Seed")
    , _node(node)
    , _project(project)
    , _renderer(renderer)
    , _oldSeed(oldSeed)
    , _newSeed(newSeed)
{}

void RandomSeedChangeCommand::redo()
{
    if (_firstRedo) {
        _firstRedo = false;
        return;
    }
    _node->randomSeed = _newSeed;
    _project->markNodeAsDirty(_node);
    if (_renderer)
        _renderer->update();
}

void RandomSeedChangeCommand::undo()
{
    _node->randomSeed = _oldSeed;
    _project->markNodeAsDirty(_node);
    if (_renderer)
        _renderer->update();
}

// ---------------------------------------------------------------------------
// AddFrameCommand
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// AddCommentCommand
// ---------------------------------------------------------------------------

AddCommentCommand::AddCommentCommand(TextureProjectPtr project,
                                     nodegraph::ScenePtr scene,
                                     const QString& commentId,
                                     QVector2D pos)
    : QUndoCommand("Add Comment")
    , _project(project)
    , _scene(scene)
    , _commentId(commentId)
    , _pos(pos)
{}

void AddCommentCommand::redo()
{
    auto modelComment   = CommentPtr(new Comment());
    modelComment->id    = _commentId;
    modelComment->pos   = _pos;
    _project->comments[_commentId] = modelComment;

    auto gcomment = nodegraph::Comment::create();
    gcomment->setId(_commentId);
    gcomment->setPos(_pos.x(), _pos.y());
    _scene->addComment(gcomment);
}

void AddCommentCommand::undo()
{
    auto gcomment = _scene->getCommentById(_commentId);
    if (gcomment)
        _scene->removeComment(gcomment);
    _project->comments.remove(_commentId);
}

// ---------------------------------------------------------------------------
// EditFrameCommand
// ---------------------------------------------------------------------------

EditFrameCommand::EditFrameCommand(FramePtr frame,
                                   nodegraph::ScenePtr scene,
                                   const QString& oldTitle, const QColor& oldColor,
                                   const QString& newTitle, const QColor& newColor)
    : QUndoCommand("Edit Frame")
    , _frame(frame)
    , _scene(scene)
    , _frameId(frame->id)
    , _oldTitle(oldTitle), _newTitle(newTitle)
    , _oldColor(oldColor), _newColor(newColor)
{}

bool EditFrameCommand::mergeWith(const QUndoCommand* other)
{
    auto* cmd = static_cast<const EditFrameCommand*>(other);
    if (cmd->_frameId != _frameId)
        return false;
    _newTitle = cmd->_newTitle;
    _newColor = cmd->_newColor;
    return true;
}

void EditFrameCommand::apply(const QString& title, const QColor& color)
{
    _frame->text  = title;
    _frame->color = color;
    auto gframe   = _scene->getFrameById(_frameId);
    if (gframe) {
        gframe->setTitle(title);
        gframe->setColor(color);
    }
}

void EditFrameCommand::redo()
{
    if (_firstRedo) { _firstRedo = false; return; }
    apply(_newTitle, _newColor);
}

void EditFrameCommand::undo()
{
    apply(_oldTitle, _oldColor);
}

// ---------------------------------------------------------------------------
// EditCommentCommand
// ---------------------------------------------------------------------------

EditCommentCommand::EditCommentCommand(CommentPtr comment,
                                       nodegraph::ScenePtr scene,
                                       const QString& oldText,
                                       const QString& newText)
    : QUndoCommand("Edit Comment")
    , _comment(comment)
    , _scene(scene)
    , _commentId(comment->id)
    , _oldText(oldText)
    , _newText(newText)
{}

bool EditCommentCommand::mergeWith(const QUndoCommand* other)
{
    auto* cmd = static_cast<const EditCommentCommand*>(other);
    if (cmd->_commentId != _commentId)
        return false;
    _newText = cmd->_newText;
    return true;
}

void EditCommentCommand::apply(const QString& text)
{
    _comment->text = text;
    auto gcomment  = _scene->getCommentById(_commentId);
    if (gcomment)
        gcomment->setText(text);
}

void EditCommentCommand::redo()
{
    if (_firstRedo) { _firstRedo = false; return; }
    apply(_newText);
}

void EditCommentCommand::undo()
{
    apply(_oldText);
}

// ---------------------------------------------------------------------------
// PasteCommand
// ---------------------------------------------------------------------------

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
        return; // nothing to paste

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
        _project->connections[con->id] = con;
        con->rightNode->isDirty = true;
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

    if (_renderer)
        _renderer->update();
}

void PasteCommand::undo()
{
    for (auto& con : _connections)
        _project->connections.remove(con->id);

    for (auto& node : _nodes) {
        auto gnode = _scene->getNodeById(node->id);
        if (gnode)
            _scene->removeNode(gnode);
        _project->nodes.remove(node->id);
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

    if (_renderer)
        _renderer->update();
}

// ---------------------------------------------------------------------------
// TextureChannelAssignCommand
// ---------------------------------------------------------------------------

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
