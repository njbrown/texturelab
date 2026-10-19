#include "variables.h"
#include "jsonutils.h"
#include "props.h"

#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <algorithm>

TextureVariable::~TextureVariable() { delete prop; }

QString TextureVariable::name() const { return prop ? prop->name : QString(); }

QString TextureVariable::displayName() const
{
    return prop ? prop->displayName : QString();
}

VariableType TextureVariable::typeForProp(const Prop* prop)
{
    if (!prop)
        return VariableType::None;
    switch (prop->type) {
    case PropType::Float:
        return VariableType::Float;
    case PropType::Int:
        return VariableType::Int;
    case PropType::Bool:
        return VariableType::Bool;
    case PropType::Enum:
        return VariableType::Enum;
    case PropType::Color:
        return VariableType::Color;
    case PropType::Gradient:
        return VariableType::Gradient;
    case PropType::Curve:
        return VariableType::Curve;
    case PropType::Image:
        return VariableType::Image;
    case PropType::String:
        return VariableType::String;
    default:
        return VariableType::None;
    }
}

QString TextureVariable::typeToString(VariableType type)
{
    switch (type) {
    case VariableType::Float:
        return "float";
    case VariableType::Int:
        return "int";
    case VariableType::Bool:
        return "bool";
    case VariableType::Enum:
        return "enum";
    case VariableType::Color:
        return "color";
    case VariableType::Gradient:
        return "gradient";
    case VariableType::Curve:
        return "curve";
    case VariableType::Image:
        return "image";
    case VariableType::String:
        return "string";
    default:
        return QString();
    }
}

VariableType TextureVariable::typeFromJson(const QJsonValue& value)
{
    if (value.isString()) {
        auto str = value.toString().trimmed().toLower();
        for (int t = (int)VariableType::Float; t <= (int)VariableType::String; t++)
            if (typeToString((VariableType)t) == str)
                return (VariableType)t;
        return VariableType::None;
    }
    // Electron 0.3 files: numeric DesignerVariableType (1-5 only).
    int number = jsonutils::getInt(value, 0);
    if (number >= (int)VariableType::Float && number <= (int)VariableType::Color)
        return (VariableType)number;
    return VariableType::None;
}

QString TextureVariable::typeLabel(VariableType type)
{
    switch (type) {
    case VariableType::Float:
        return "Float";
    case VariableType::Int:
        return "Int";
    case VariableType::Bool:
        return "Bool";
    case VariableType::Enum:
        return "Enum";
    case VariableType::Color:
        return "Color";
    case VariableType::Gradient:
        return "Gradient";
    case VariableType::Curve:
        return "Curve";
    case VariableType::Image:
        return "Image";
    case VariableType::String:
        return "String";
    default:
        return "None";
    }
}

static Prop* createPropForType(VariableType type)
{
    switch (type) {
    case VariableType::Float:
        return new FloatProp();
    case VariableType::Int:
        return new IntProp();
    case VariableType::Bool:
        return new BoolProp();
    case VariableType::Enum:
        return new EnumProp();
    case VariableType::Color:
        return new ColorProp();
    case VariableType::Gradient:
        return new GradientProp();
    case VariableType::Curve:
        return new CurveProp();
    case VariableType::Image:
        return new ImageProp();
    case VariableType::String:
        return new StringProp();
    default:
        return nullptr;
    }
}

// A copy of a prop for a variable to own. An ImageProp copy would share the
// node's GL texture pointer; the variable never renders, so drop it.
static Prop* cloneForVariable(const Prop* source)
{
    Prop* copy = source->clone();
    if (auto image = dynamic_cast<ImageProp*>(copy)) {
        image->texture = nullptr;
        image->_textureDirty = true;
    }
    return copy;
}

TextureVariablePtr TextureVariable::createFromProp(const Prop* source,
                                                   const QString& name,
                                                   const QString& displayName)
{
    auto type = typeForProp(source);
    if (type == VariableType::None)
        return {};

    TextureVariablePtr var(new TextureVariable());
    var->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    var->type = type;
    var->prop = cloneForVariable(source);
    var->prop->name = name;
    var->prop->displayName = displayName;
    var->prop->order = 0;
    var->clampValue();
    return var;
}

bool TextureVariable::accepts(const Prop* target) const
{
    if (!prop || typeForProp(target) != type)
        return false;
    if (type == VariableType::Enum) {
        auto mine = static_cast<const EnumProp*>(prop);
        auto theirs = static_cast<const EnumProp*>(target);
        return mine->values == theirs->values;
    }
    return true;
}

bool TextureVariable::hasLink(const QString& nodeId,
                              const QString& propName) const
{
    return links.contains(VariableLink{nodeId, propName});
}

QVariant TextureVariable::value() const
{
    return prop ? prop->getValue() : QVariant();
}

void TextureVariable::setValue(const QVariant& value)
{
    if (!prop)
        return;
    prop->setValue(value);
    clampValue();
}

void TextureVariable::clampValue()
{
    if (!prop)
        return;

    switch (type) {
    case VariableType::Float: {
        auto p = static_cast<FloatProp*>(prop);
        if (p->minValue <= p->maxValue)
            p->value = std::clamp(p->value, p->minValue, p->maxValue);
        break;
    }
    case VariableType::Int: {
        auto p = static_cast<IntProp*>(prop);
        if (p->minValue <= p->maxValue)
            p->value = std::clamp(p->value, p->minValue, p->maxValue);
        break;
    }
    case VariableType::Enum: {
        auto p = static_cast<EnumProp*>(prop);
        if (p->values.isEmpty()) {
            p->index = 0;
            break;
        }
        p->index = std::clamp(p->index, 0, (int)p->values.size() - 1);
        if (!options.isEmpty()) {
            bool enabled = std::any_of(options.begin(), options.end(),
                                       [&](const EnumOption& o) {
                                           return o.index == p->index;
                                       });
            if (!enabled)
                p->index = options.first().index;
        }
        break;
    }
    default:
        break;
    }
}

QList<EnumOption> TextureVariable::effectiveOptions() const
{
    if (type != VariableType::Enum || !prop)
        return {};
    if (!options.isEmpty())
        return options;

    QList<EnumOption> all;
    auto p = static_cast<const EnumProp*>(prop);
    for (int i = 0; i < p->values.size(); i++)
        all.append({i, p->values[i]});
    return all;
}

void TextureVariable::writeTo(Prop* target) const
{
    if (target && prop && accepts(target))
        target->setValue(prop->getValue());
}

QJsonObject TextureVariable::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["type"] = typeToString(type);

    QJsonObject propObj = prop ? prop->toJson() : QJsonObject();
    propObj["name"] = name();
    propObj["displayName"] = displayName();
    obj["property"] = propObj;

    QJsonArray linkArray;
    for (const auto& link : links) {
        QJsonObject l;
        l["nodeId"] = link.nodeId;
        l["name"] = link.propName;
        linkArray.append(l);
    }
    obj["linkedProperties"] = linkArray;

    if (type == VariableType::Enum && !options.isEmpty()) {
        QJsonArray optionArray;
        for (const auto& option : options) {
            QJsonObject o;
            o["index"] = option.index;
            o["label"] = option.label;
            optionArray.append(o);
        }
        obj["options"] = optionArray;
    }

    return obj;
}

TextureVariablePtr TextureVariable::fromJson(const QJsonObject& obj)
{
    auto type = typeFromJson(obj["type"]);
    Prop* prop = createPropForType(type);
    if (!prop)
        return {};

    TextureVariablePtr var(new TextureVariable());
    var->type = type;
    var->prop = prop;
    var->id = jsonutils::getString(obj["id"]);
    if (var->id.isEmpty())
        var->id = QUuid::createUuid().toString(QUuid::WithoutBraces);

    auto propObj = obj["property"].toObject();
    prop->fromJson(propObj);
    prop->name = jsonutils::getString(propObj["name"]);
    prop->displayName = jsonutils::getString(propObj["displayName"]);
    if (prop->displayName.isEmpty())
        prop->displayName = prop->name;

    for (const auto& item : obj["linkedProperties"].toArray()) {
        auto l = item.toObject();
        VariableLink link{jsonutils::getString(l["nodeId"]),
                          jsonutils::getString(l["name"])};
        if (!link.nodeId.isEmpty() && !link.propName.isEmpty() &&
            !var->links.contains(link))
            var->links.append(link);
    }

    for (const auto& item : obj["options"].toArray()) {
        auto o = item.toObject();
        var->options.append({jsonutils::getInt(o["index"]),
                             jsonutils::getString(o["label"])});
    }

    return var;
}

TextureVariablePtr TextureVariable::clone() const
{
    TextureVariablePtr copy(new TextureVariable());
    copy->id = id;
    copy->type = type;
    copy->prop = prop ? cloneForVariable(prop) : nullptr;
    copy->options = options;
    copy->links = links;
    return copy;
}

namespace variables {

QString sanitizeName(const QString& displayName)
{
    QString name;
    bool lastUnderscore = false;
    for (QChar c : displayName.trimmed().toLower()) {
        if (c.isLetterOrNumber() && c.unicode() < 128) {
            name += c;
            lastUnderscore = false;
        }
        else if (!lastUnderscore && !name.isEmpty()) {
            name += '_';
            lastUnderscore = true;
        }
    }
    while (name.endsWith('_'))
        name.chop(1);
    if (name.isEmpty())
        name = "variable";
    if (name[0].isDigit())
        name.prepend('_');
    return name;
}

QString uniqueName(const QString& base, const QStringList& taken)
{
    if (!taken.contains(base))
        return base;
    for (int i = 2;; i++) {
        auto candidate = QString("%1_%2").arg(base).arg(i);
        if (!taken.contains(candidate))
            return candidate;
    }
}

static bool isValidName(const QString& name)
{
    static const QRegularExpression re("^[A-Za-z_][A-Za-z0-9_]*$");
    return re.match(name).hasMatch();
}

void validate(QList<TextureVariablePtr>& vars, const PropResolver& resolve)
{
    QSet<QString> ids;
    QStringList names;
    QSet<QString> linkedProps; // "nodeId/propName"

    for (int i = 0; i < vars.size();) {
        auto var = vars[i];
        if (!var || !var->prop) {
            vars.removeAt(i);
            continue;
        }

        if (ids.contains(var->id))
            var->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        ids.insert(var->id);

        auto name = var->name();
        if (!isValidName(name))
            name = sanitizeName(name.isEmpty() ? var->displayName() : name);
        name = uniqueName(name, names);
        var->prop->name = name;
        names.append(name);
        if (var->prop->displayName.isEmpty())
            var->prop->displayName = name;

        // Enum options must point into the option list, once each.
        if (var->type == VariableType::Enum) {
            auto count = static_cast<EnumProp*>(var->prop)->values.size();
            QList<EnumOption> valid;
            QSet<int> seen;
            for (auto option : var->options) {
                if (option.index < 0 || option.index >= count ||
                    seen.contains(option.index))
                    continue;
                seen.insert(option.index);
                if (option.label.isEmpty())
                    option.label =
                        static_cast<EnumProp*>(var->prop)->values[option.index];
                valid.append(option);
            }
            var->options = valid;
        }

        QList<VariableLink> links;
        for (const auto& link : var->links) {
            auto key = link.nodeId + "/" + link.propName;
            if (linkedProps.contains(key))
                continue;
            Prop* target = resolve ? resolve(link.nodeId, link.propName) : nullptr;
            if (!target || !var->accepts(target))
                continue;
            linkedProps.insert(key);
            links.append(link);
        }
        var->links = links;

        var->clampValue();
        i++;
    }
}

QJsonArray toJson(const QList<TextureVariablePtr>& vars)
{
    QJsonArray array;
    for (const auto& var : vars)
        if (var)
            array.append(var->toJson());
    return array;
}

QList<TextureVariablePtr> fromJson(const QJsonArray& array)
{
    QList<TextureVariablePtr> vars;
    for (const auto& item : array) {
        if (!item.isObject())
            continue;
        auto var = TextureVariable::fromJson(item.toObject());
        if (var)
            vars.append(var);
    }
    return vars;
}

QList<TextureVariablePtr> cloneAll(const QList<TextureVariablePtr>& vars)
{
    QList<TextureVariablePtr> copies;
    for (const auto& var : vars)
        if (var)
            copies.append(var->clone());
    return copies;
}

} // namespace variables
