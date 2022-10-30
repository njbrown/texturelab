#include "models.h"
#include "libraries/library.h"
#include "props.h"

TextureNodePtr TextureProject::getNodeById(const QString& id)
{
    return nodes[id];
}

ConnectionPtr TextureProject::getConnectionById(const QString& id)
{
    return connections[id];
}

QVector<TextureNodePtr> TextureProject::getNodeDependencies(const QString& id)
{
    auto node = nodes[id];

    QVector<TextureNodePtr> cons;
    for (auto con : connections) {
        if (con->rightNode == node) {
            auto depNode = con->leftNode;
            cons.append(depNode);
        }
    }

    return cons;
}

void TextureProject::addNode(const TextureNodePtr& node)
{
    // todo: check if node already exists
    this->nodes[node->id] = node;
}

void TextureProject::addConnection(TextureNodePtr leftNode,
                                   TextureNodePtr rightNode,
                                   QString rightNodeInput)
{
    QSharedPointer<Connection> con(new Connection());
    con->id = createGuid();

    con->leftNode = leftNode;
    con->rightNode = rightNode;

    con->rightNodeInputName = rightNodeInput;

    this->connections[con->id] = con;

    // todo: request updates
}

ConnectionPtr TextureProject::removeConnection(const QString& leftNode,
                                               const QString& rightNode,
                                               const QString& rightNodeInput)
{
    for (auto conKey : connections.keys()) {
        auto con = connections[conKey];

        if (con->leftNode->id == leftNode && con->rightNode->id == rightNode &&
            con->rightNodeInputName == rightNodeInput) {
            connections.remove(conKey);

            return con;
        }
    }

    return ConnectionPtr(nullptr);
}

void TextureProject::removeConnection(ConnectionPtr con)
{
    this->connections.remove(con->id);
}

void TextureProject::removeConnection(const QString& id)
{
    this->connections.remove(id);
}

TextureProjectPtr TextureProject::createEmpty(Library* library)
{
    auto project = new TextureProject();
    project->randomSeed = 0;
    if (library != nullptr)
        project->library = library;
    else
        project->library = createLibraryV2();

    return TextureProjectPtr(project);
}

TextureNode::TextureNode()
{
    id = createGuid();
    isDirty = true;
}

void TextureNode::addInput(const QString& inputName)
{
    inputs.append(inputName);
}

void TextureNode::setProp(QString propName, QVariant value)
{
    if (props.contains(propName))
        props[propName]->setValue(value);
}