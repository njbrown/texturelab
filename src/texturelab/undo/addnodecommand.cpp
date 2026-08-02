#include "addnodecommand.h"

#include "../graphics/texturerenderer.h"
#include "../libraries/library.h"
#include "graph/scene.h"

#include <QUuid>

static QString newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

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
    node->id  = _nodeId;
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

    // also drops the node's connections, marking every downstream chain dirty
    _project->removeNode(_nodeId);
    if (_renderer)
        _renderer->update();
}
