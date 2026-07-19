#include "models.h"
#include "libraries/library.h"
#include "props.h"
#include <QOpenGLFramebufferObject>
#include <QQueue>
#include <QRandomGenerator>

TextureNodePtr TextureProject::getNodeById(const QString& id)
{
    // .value() (not operator[]): a read-only lookup must never default-insert a
    // null entry into the map, which the renderer would later dereference.
    return nodes.value(id);
}

ConnectionPtr TextureProject::getConnectionById(const QString& id)
{
    return connections.value(id);
}

QVector<TextureNodePtr> TextureProject::getNodeDependencies(const QString& id)
{
    auto node = nodes.value(id);

    QVector<TextureNodePtr> cons;
    for (auto con : connections) {
        if (con->rightNode == node) {
            auto depNode = con->leftNode;
            cons.append(depNode);
        }
    }

    return cons;
}

QVector<TextureNodePtr> TextureProject::getNodeRightOfNode(const QString& id)
{
    auto node = nodes.value(id);

    QVector<TextureNodePtr> cons;
    for (auto con : connections) {
        if (con->leftNode == node) {
            auto nextNode = con->rightNode;
            cons.append(nextNode);
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

        if (!con || !con->leftNode || !con->rightNode)
            continue;

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

void TextureProject::markNodeAsDirty(const TextureNodePtr& node)
{
    QQueue<TextureNodePtr> queue;
    queue.enqueue(node);

    while (!queue.isEmpty()) {
        auto nextNode = queue.dequeue();
        nextNode->isDirty = true;

        auto list = getNodeRightOfNode(nextNode->id);
        for (auto item : list)
            queue.enqueue(item);
    }
}

TextureProjectPtr TextureProject::createEmpty(Library* library)
{
    auto project = new TextureProject();
    project->randomSeed = 0;
    if (library != nullptr)
        project->library = library;
    else
        project->library = createLibraryV3();

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
    if (props.contains(propName)) {
        props[propName]->setValue(value);
        this->isDirty = true;
    }
}

Prop* TextureNode::getProp(QString propName)
{
    if (props.contains(propName)) {
        return props[propName];
    }

    return nullptr;
}

bool TextureNode::hasProp(QString propName) { return props.contains(propName); }

unsigned int TextureNode::textureId()
{
    // texture is null until the node's graphics resources are initialized;
    // return 0 (the GL "no texture" name) rather than dereferencing null.
    return this->texture ? this->texture->texture() : 0;
}

PropertyGroup* TextureNode::createGroup(const QString& name)
{
    auto group = new PropertyGroup();
    group->name = name;
    this->propertyGroups.append(group);

    return group;
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
    prop->order = props.size();

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
    prop->order = props.size();

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
    prop->order = props.size();

    props[name] = prop;

    return prop;
}

EnumProp* TextureNode::addEnumProp(const QString& name,
                                   const QString& displayName,
                                   QList<QString> values)
{
    auto prop = new EnumProp();
    prop->name = name;
    prop->displayName = displayName;
    prop->values = values;
    prop->index = 0;
    prop->order = props.size();

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
    prop->order = props.size();

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
    prop->order = props.size();

    props[name] = prop;

    return prop;
}

GradientProp* TextureNode::addGradientProp(const QString& name,
                                           const QString& displayName,
                                           const Gradient& defaultVal)
{
    auto prop = new GradientProp();
    prop->name = name;
    prop->displayName = displayName;
    prop->setValue(QVariant::fromValue(Gradient(defaultVal)));
    prop->order = props.size();

    props[name] = prop;

    return prop;
}

ImageProp* TextureNode::addImageProp(const QString& name,
                                     const QString& displayName)
{
    auto prop = new ImageProp();
    prop->name = name;
    prop->displayName = displayName;
    prop->order = props.size();

    props[name] = prop;

    return prop;
}

CurveProp* TextureNode::addCurveProp(const QString& name,
                                     const QString& displayName)
{
    auto prop = new CurveProp();
    prop->name = name;
    prop->displayName = displayName;
    prop->order = props.size();

    props[name] = prop;

    return prop;
}