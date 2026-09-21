#include "propertychangecommand.h"

#include "../graphics/texturerenderer.h"

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
