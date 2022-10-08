#include "models.h"
#include "props.h"

TextureNodePtr TextureProject::getNodeById(const QString &id)
{
    return nodes[id];
}

void TextureProject::addConnection(TextureNodePtr leftNode, TextureNodePtr rightNode, QString rightNodeInput)
{
    auto con = ConnectionPtr(new Connection());
    con->id = createGuid();

    con->leftNode = leftNode;
    con->rightNode = rightNode;

    con->rightNodeInputName = rightNodeInput;

    this->connections[con->id] = con;

    // todo: request updates
}

void TextureNode::setProp(QString propName, QVariant value)
{
    if (props.contains(propName))
        props[propName]->setValue(value);
}