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

IntProp* TextureNode::addIntProp(const QString& name,
                                 const QString& displayName, int defaultVal,
                                 int minVal, int maxVal, int increment)
{
    auto prop = new IntProp();
    prop->name = name;
    prop->displayName = displayName;
    prop->minValue = minVal;
    prop->maxValue = maxVal;
    prop->step = increment;
    prop->value = defaultVal;

    props[name] = prop;

    return prop;
}

FloatProp* TextureNode::addFloatProp(const QString& name,
                                     const QString& displayName,
                                     double defaultVal, double minVal,
                                     double maxVal, double increment)
{
    auto prop = new FloatProp();
    prop->name = name;
    prop->displayName = displayName;
    prop->minValue = minVal;
    prop->maxValue = maxVal;
    prop->step = increment;
    prop->value = defaultVal;

    props[name] = prop;

    return prop;
}

BoolProp* TextureNode::addBoolProp(const QString& name,
                                   const QString& displayName, bool defaultVal)
{
    auto prop = new BoolProp();
    prop->name = name;
    prop->displayName = displayName;
    prop->value = defaultVal;

    props[name] = prop;

    return prop;
}

EnumProp* TextureNode::addEnumProp(const QString& name,
                                   const QString& displayName,
                                   QList<QString> defaultVal)
{
    auto prop = new EnumProp();
    prop->name = name;
    prop->displayName = displayName;
    prop->values = defaultVal;

    props[name] = prop;

    return prop;
}

ColorProp* TextureNode::addColorProp(const QString& name,
                                     const QString& displayName,
                                     const QColor& defaultVal)
{
    auto prop = new ColorProp();
    prop->name = name;
    prop->displayName = displayName;
    prop->value = defaultVal;

    props[name] = prop;

    return prop;
}

StringProp* TextureNode::addStringProp(const QString& name,
                                       const QString& displayName,
                                       const QString& defaultVal)
{
    auto prop = new StringProp();
    prop->name = name;
    prop->displayName = displayName;
    prop->value = defaultVal;

    props[name] = prop;

    return prop;
}

// GradientProp* addGradientProp(const QString& name, const QString&
// displayName,
//                               Gradient defaultVal);

ImageProp* TextureNode::addImageProp(const QString& name,
                                     const QString& displayName)
{
    auto prop = new ImageProp();
    prop->name = name;
    prop->displayName = displayName;

    props[name] = prop;

    return prop;
}