#include "variableschangecommand.h"

#include "../graphics/texturerenderer.h"
#include "../props.h"

static QString propKey(const QString& nodeId, const QString& propName)
{
    return nodeId + "/" + propName;
}

VariablesChangeCommand::VariablesChangeCommand(TextureProjectPtr project,
                                               TextureRenderer* renderer,
                                               const QString& text,
                                               const QString& mergeKey,
                                               QJsonArray before,
                                               QJsonArray after,
                                               PropValues propsBefore,
                                               PropValues propsAfter)
    : QUndoCommand(text)
    , _project(project)
    , _renderer(renderer)
    , _mergeKey(mergeKey)
    , _before(before)
    , _after(after)
    , _propsBefore(propsBefore)
    , _propsAfter(propsAfter)
{}

bool VariablesChangeCommand::mergeWith(const QUndoCommand* other)
{
    auto* cmd = static_cast<const VariablesChangeCommand*>(other);
    if (cmd->_mergeKey != _mergeKey ||
        !UndoMerge::withinWindow(_editedAt, cmd->_editedAt))
        return false;

    _after = cmd->_after;
    // Keep the earliest "before" of each prop; take the latest "after".
    for (auto it = cmd->_propsBefore.begin(); it != cmd->_propsBefore.end(); ++it)
        if (!_propsBefore.contains(it.key()))
            _propsBefore.insert(it.key(), it.value());
    for (auto it = cmd->_propsAfter.begin(); it != cmd->_propsAfter.end(); ++it)
        _propsAfter.insert(it.key(), it.value());
    _editedAt = cmd->_editedAt;
    return true;
}

void VariablesChangeCommand::apply(const QJsonArray& vars,
                                   const PropValues& props)
{
    _project->variables = variables::fromJson(vars);

    for (auto it = props.begin(); it != props.end(); ++it) {
        auto slash = it.key().indexOf('/');
        auto node = _project->getNodeById(it.key().left(slash));
        if (!node)
            continue;
        auto prop = node->getProp(it.key().mid(slash + 1));
        if (!prop)
            continue;
        prop->fromJsonValue(it.value());
        _project->markNodeAsDirty(node);
    }

    if (_renderer)
        _renderer->update();
}

void VariablesChangeCommand::redo()
{
    if (_firstRedo) {
        _firstRedo = false;
        return; // already applied by the caller
    }
    apply(_after, _propsAfter);
}

void VariablesChangeCommand::undo() { apply(_before, _propsBefore); }

// ---------------------------------------------------------------------------

VariablesEdit::VariablesEdit(TextureProjectPtr project,
                             TextureRenderer* renderer,
                             const QString& text,
                             const QString& mergeKey)
    : _project(project)
    , _renderer(renderer)
    , _text(text)
    , _mergeKey(mergeKey)
    , _before(variables::toJson(project->variables))
{}

void VariablesEdit::touchProp(const QString& nodeId, const QString& propName)
{
    auto key = propKey(nodeId, propName);
    if (_propsBefore.contains(key))
        return;
    if (auto prop = _project->resolveProp(nodeId, propName))
        _propsBefore.insert(key, prop->toJsonValue());
}

void VariablesEdit::touchVariable(const TextureVariablePtr& var)
{
    if (!var)
        return;
    for (const auto& link : var->links)
        touchProp(link.nodeId, link.propName);
}

VariablesChangeCommand* VariablesEdit::finish()
{
    VariablesChangeCommand::PropValues after;
    for (auto it = _propsBefore.begin(); it != _propsBefore.end(); ++it) {
        auto slash = it.key().indexOf('/');
        if (auto prop = _project->resolveProp(it.key().left(slash),
                                              it.key().mid(slash + 1)))
            after.insert(it.key(), prop->toJsonValue());
    }
    return new VariablesChangeCommand(_project, _renderer, _text, _mergeKey,
                                      _before,
                                      variables::toJson(_project->variables),
                                      _propsBefore, after);
}
